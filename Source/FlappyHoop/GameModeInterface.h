// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "BallType.h"
#include "Ads/AGRewardItem.h"
#include "GameStateEnum.h"
#include "GameModeInterface.generated.h"

class IGameInstanceInterface;

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UGameModeInterface : public UInterface
{
	GENERATED_BODY()
};

DECLARE_MULTICAST_DELEGATE(FOnGameStarted);
DECLARE_MULTICAST_DELEGATE(FOnViewportFetched);
DECLARE_MULTICAST_DELEGATE(FOnGameReset);
DECLARE_MULTICAST_DELEGATE(FOnPointScored);
DECLARE_MULTICAST_DELEGATE(FOnTimeEnded);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCoinCollected, bool);
DECLARE_MULTICAST_DELEGATE(FOnSecondChanceGranted);

class FLAPPYHOOP_API IGameModeInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	virtual FOnGameStarted& OnGameStartedDelegate() = 0;
	virtual FOnViewportFetched& OnViewportFetchedDelegate() = 0;
	virtual FOnGameReset& OnGameResetDelegate() = 0;
	virtual FVector2D GetViewportSize() const = 0;
	virtual void ResetGame() = 0;
	virtual float GetMaxGameTime() const = 0;
	virtual void SetNewGameTime() = 0;
	virtual FOnPointScored& OnPointScoredDelegate() = 0;
	virtual void UpdateScore() = 0;
	virtual void UpdateScoreMultiplier() = 0;
	virtual void ResetScoreMultiplier() = 0;
	virtual int GetScoreMultiplier() const = 0;
	virtual int GetCurrentScore() = 0;
	virtual FOnTimeEnded& OnTimeEndedDelegate() = 0;
	virtual bool GetTimeEndedBool() const = 0;
	virtual void SetTimeEndedBool(bool NewTimeEnded) = 0;
	virtual void EndGame() = 0;
	virtual int GetHighScore() const = 0;
	virtual void ApplyBallSettings() = 0;
	virtual void SetBallType(EBallType NewBallType) = 0;
	virtual EBallType GetBallType() const = 0;
	virtual FOnCoinCollected& OnCoinCollectedDelegate() = 0;
	virtual int32 GetTotalCoins() const = 0;
	virtual int32 GetTotalGems() const = 0;
	virtual void AddCoin() = 0;
	virtual void AddGem() = 0;
	virtual int32 GetCollectedCoins() const = 0;
	virtual int32 GetCollectedGems() const = 0;
	virtual void ActivateCoin() = 0;
	virtual IGameInstanceInterface* GetGameInstanceInterface() const = 0;
	virtual FOnSecondChanceGranted& OnSecondChanceGrantedDelegate() = 0;
	virtual void LoadRewardedAd() = 0;
	virtual void CreateSecondChanceWidget() = 0;
	virtual bool GetCanWatchAd() const = 0;
	virtual void SetCanWatchAd(bool bNewCanWatchAd) = 0;
	virtual int32 GetGemsNeededForSecondChance() const = 0;
	virtual void IncrementRetryCount() = 0;
	virtual void IncrementGemsNeededForSecondChance() = 0;
	virtual void GrantSecondChance(FRewardItem Reward) = 0;
	virtual int32 GetGemsSpent() const = 0;
	virtual void IncrementGemsSpentCount(int32 IncrementBy) = 0;
	virtual FTimerHandle& GetSwitchSidesTimerHandle() = 0;
	virtual void StopInterstitialTimer() = 0;
	virtual EGameStateEnum GetCurrentGameState() const = 0;
	virtual void SetCurrentGameState(EGameStateEnum NewState) = 0;
};
