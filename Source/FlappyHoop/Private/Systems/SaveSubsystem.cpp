// Fill out your copyright notice in the Description page of Project Settings.

#include "Systems/SaveSubsystem.h"
#include "Core/FlappyHoop.h"
#include "Data/MySaveGame.h"
#include "Framework/GameInfoInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

const FString USaveSubsystem::SlotName = TEXT("UserProgressionSlot");

USaveSubsystem* USaveSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<USaveSubsystem>() : nullptr;
}

void USaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Load();
}

void USaveSubsystem::Deinitialize()
{
	// Make sure the latest progression is on disk before exit.
	if (SaveGame && (bSaveDirty || bSaveInFlight))
	{
		SaveGame->UserProgression = Progression;
		UGameplayStatics::SaveGameToSlot(SaveGame, SlotName, 0);
	}

	Super::Deinitialize();
}

void USaveSubsystem::Load()
{
	if (UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		SaveGame = Cast<UMySaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
	}

	if (!SaveGame)
	{
		// No save yet, or it failed to load (e.g. corrupted): start fresh from the save class defaults.
		const UGameInfoInstance* GameInstance = Cast<UGameInfoInstance>(GetGameInstance());
		TSubclassOf<UMySaveGame> SaveClass = GameInstance ? GameInstance->GetSaveGameClass() : nullptr;
		if (!SaveClass)
		{
			SaveClass = UMySaveGame::StaticClass();
		}
		SaveGame = Cast<UMySaveGame>(UGameplayStatics::CreateSaveGameObject(SaveClass));
	}

	if (SaveGame)
	{
		Progression = SaveGame->UserProgression;
		MigrateSave();
	}
}

void USaveSubsystem::MigrateSave()
{
	constexpr int32 CurrentVersion = 1;
	if (Progression.SaveVersion >= CurrentVersion)
	{
		return;
	}

	// v0 -> v1: the save carried the whole shop list; keep only the types that were bought.
	for (const FBallsShopStruct& Legacy : Progression.BallsOwned)
	{
		if (Legacy.IsPurchased)
		{
			Progression.OwnedBalls.AddUnique(Legacy.BallType);
		}
	}

	UE_LOG(LogFlappyHoop, Log, TEXT("Save migrated to v%d (%d owned balls)"), CurrentVersion, Progression.OwnedBalls.Num());

	Progression.BallsOwned.Empty();
	Progression.SaveVersion = CurrentVersion;
	Save(Progression);
}

void USaveSubsystem::Save(const FUserProgression& NewProgression)
{
	Progression = NewProgression;

	if (SaveGame)
	{
		SaveGame->UserProgression = Progression;
		WriteToDisk();
	}
}

void USaveSubsystem::WriteToDisk()
{
	if (bSaveInFlight)
	{
		// Two concurrent async writes to one slot aren't ordered and can corrupt the file. Coalesce instead.
		bSaveDirty = true;
		return;
	}

	bSaveInFlight = true;
	bSaveDirty = false;

	// Serialization happens now on the game thread; disk IO happens off-thread (no hitch on mobile).
	UGameplayStatics::AsyncSaveGameToSlot(SaveGame, SlotName, 0,
		FAsyncSaveGameToSlotDelegate::CreateUObject(this, &USaveSubsystem::OnAsyncSaveFinished));
}

void USaveSubsystem::OnAsyncSaveFinished(const FString& Slot, const int32 UserIndex, bool bSuccess)
{
	bSaveInFlight = false;

	if (!bSuccess)
	{
		UE_LOG(LogFlappyHoop, Warning, TEXT("Saving %s failed; will retry on next change."), *Slot);
	}

	if (bSaveDirty && SaveGame)
	{
		SaveGame->UserProgression = Progression; // write the newest state
		WriteToDisk();
	}
}
