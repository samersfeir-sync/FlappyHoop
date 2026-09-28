// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "EconomySubsystem.generated.h"

class USaveSubsystem;

/** Params: new coin total, new gem total. */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnBalanceChanged, int32 /*Coins*/, int32 /*Gems*/);

/**
 * Single place that reads and changes the player's banked coins and gems.
 * Every change is saved immediately (async) and broadcast through OnBalanceChanged.
 */
UCLASS()
class FLAPPYHOOP_API UEconomySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	static UEconomySubsystem* Get(const UObject* WorldContextObject);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	int32 GetCoins() const;
	int32 GetGems() const;

	void AddCoins(int32 Amount);
	void AddGems(int32 Amount);

	bool CanAfford(int32 Coins, int32 Gems) const;

	/** Spends both amounts, or nothing if either can't be afforded. */
	bool TrySpend(int32 Coins, int32 Gems);

	FOnBalanceChanged OnBalanceChanged;

private:

	USaveSubsystem* GetSave() const;

	/** Applies the deltas (clamped at 0), saves and broadcasts. */
	void ApplyDelta(int32 CoinDelta, int32 GemDelta);
};
