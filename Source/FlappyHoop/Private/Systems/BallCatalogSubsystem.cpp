// Fill out your copyright notice in the Description page of Project Settings.

#include "Systems/BallCatalogSubsystem.h"
#include "Core/FlappyHoop.h"
#include "Data/MySaveGame.h"
#include "Framework/GameInfoInstance.h"
#include "Systems/SaveSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

UBallCatalogSubsystem* UBallCatalogSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UBallCatalogSubsystem>() : nullptr;
}

void UBallCatalogSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Collection.InitializeDependency<USaveSubsystem>();
	Super::Initialize(Collection);

	BuildBallList();
	ValidateSave();
}

USaveSubsystem* UBallCatalogSubsystem::GetSave() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<USaveSubsystem>() : nullptr;
}

void UBallCatalogSubsystem::BuildBallList()
{
	Balls.Reset();

	const UGameInfoInstance* GameInstance = Cast<UGameInfoInstance>(GetGameInstance());
	if (const UBallCatalog* Catalog = GameInstance ? GameInstance->GetBallCatalog() : nullptr)
	{
		Balls = Catalog->Balls;
		bLegacyData = false;
		return;
	}

	// Fallback: build the list from the ball array on the save-game Blueprint's defaults (the old source of truth).
	bLegacyData = true;
	UE_LOG(LogFlappyHoop, Warning, TEXT("No BallCatalog assigned on the game instance; using the save-game defaults for the shop."));

	const TSubclassOf<UMySaveGame> SaveClass = GameInstance ? GameInstance->GetSaveGameClass() : nullptr;
	const UMySaveGame* Defaults = SaveClass ? SaveClass->GetDefaultObject<UMySaveGame>() : nullptr;
	if (!Defaults)
	{
		return;
	}

	for (const FBallsShopStruct& Legacy : Defaults->UserProgression.BallsOwned)
	{
		FBallCatalogEntry Entry;
		Entry.BallType = Legacy.BallType;
		Entry.Icon = Legacy.BallTexture;
		Entry.Price = Legacy.Price;
		Entry.bOwnedByDefault = Legacy.IsPurchased;
		Balls.Add(Entry);
	}
}

const FBallCatalogEntry* UBallCatalogSubsystem::Find(EBallType BallType) const
{
	return Balls.FindByPredicate([BallType](const FBallCatalogEntry& Entry) { return Entry.BallType == BallType; });
}

bool UBallCatalogSubsystem::IsOwned(EBallType BallType) const
{
	// A ball that isn't in the catalog (any more) doesn't count as owned.
	const FBallCatalogEntry* Entry = Find(BallType);
	if (!Entry)
	{
		return false;
	}

	if (Entry->bOwnedByDefault)
	{
		return true;
	}

	const USaveSubsystem* Save = GetSave();
	return Save && Save->GetProgression().OwnedBalls.Contains(BallType);
}

EBallType UBallCatalogSubsystem::GetSelected() const
{
	const USaveSubsystem* Save = GetSave();
	return Save ? Save->GetProgression().BallType : EBallType::Basketball;
}

bool UBallCatalogSubsystem::Unlock(EBallType BallType)
{
	USaveSubsystem* Save = GetSave();
	if (!Save || !Find(BallType) || IsOwned(BallType))
	{
		return false;
	}

	FUserProgression Progression = Save->GetProgression();
	Progression.OwnedBalls.AddUnique(BallType);
	Progression.BallType = BallType;
	Save->Save(Progression);

	OnBallsChanged.Broadcast();
	return true;
}

bool UBallCatalogSubsystem::Select(EBallType BallType)
{
	USaveSubsystem* Save = GetSave();
	if (!Save || !IsOwned(BallType))
	{
		return false;
	}

	if (Save->GetProgression().BallType != BallType)
	{
		FUserProgression Progression = Save->GetProgression();
		Progression.BallType = BallType;
		Save->Save(Progression);
	}

	OnBallsChanged.Broadcast();
	return true;
}

void UBallCatalogSubsystem::ValidateSave()
{
	USaveSubsystem* Save = GetSave();
	if (!Save || Balls.IsEmpty())
	{
		return;
	}

	if (IsOwned(GetSelected()))
	{
		return;
	}

	// The selected ball isn't owned (or no longer exists): fall back to the first owned ball.
	for (const FBallCatalogEntry& Entry : Balls)
	{
		if (IsOwned(Entry.BallType))
		{
			FUserProgression Progression = Save->GetProgression();
			Progression.BallType = Entry.BallType;
			Save->Save(Progression);
			return;
		}
	}
}
