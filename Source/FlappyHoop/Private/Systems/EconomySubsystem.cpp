// Fill out your copyright notice in the Description page of Project Settings.

#include "Systems/EconomySubsystem.h"
#include "Core/FlappyHoop.h"
#include "Systems/SaveSubsystem.h"
#include "Data/UserProgression.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

UEconomySubsystem* UEconomySubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UEconomySubsystem>() : nullptr;
}

void UEconomySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Collection.InitializeDependency<USaveSubsystem>();
	Super::Initialize(Collection);
}

USaveSubsystem* UEconomySubsystem::GetSave() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<USaveSubsystem>() : nullptr;
}

int32 UEconomySubsystem::GetCoins() const
{
	const USaveSubsystem* Save = GetSave();
	return Save ? Save->GetProgression().TotalCoins : 0;
}

int32 UEconomySubsystem::GetGems() const
{
	const USaveSubsystem* Save = GetSave();
	return Save ? Save->GetProgression().TotalGems : 0;
}

void UEconomySubsystem::AddCoins(int32 Amount)
{
	if (Amount > 0)
	{
		ApplyDelta(Amount, 0);
	}
}

void UEconomySubsystem::AddGems(int32 Amount)
{
	if (Amount > 0)
	{
		ApplyDelta(0, Amount);
	}
}

bool UEconomySubsystem::CanAfford(int32 Coins, int32 Gems) const
{
	return Coins >= 0 && Gems >= 0 && GetCoins() >= Coins && GetGems() >= Gems;
}

bool UEconomySubsystem::TrySpend(int32 Coins, int32 Gems)
{
	if (!CanAfford(Coins, Gems))
	{
		return false;
	}

	if (Coins > 0 || Gems > 0)
	{
		ApplyDelta(-Coins, -Gems);
	}
	return true;
}

void UEconomySubsystem::ApplyDelta(int32 CoinDelta, int32 GemDelta)
{
	USaveSubsystem* Save = GetSave();
	if (!Save)
	{
		UE_LOG(LogFlappyHoop, Warning, TEXT("Economy: no save subsystem, change of %d coins / %d gems dropped"), CoinDelta, GemDelta);
		return;
	}

	FUserProgression Progression = Save->GetProgression();
	Progression.TotalCoins = FMath::Max(0, Progression.TotalCoins + CoinDelta);
	Progression.TotalGems = FMath::Max(0, Progression.TotalGems + GemDelta);
	Save->Save(Progression);

	OnBalanceChanged.Broadcast(Progression.TotalCoins, Progression.TotalGems);
}
