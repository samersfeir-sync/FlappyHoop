// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "GameModeInterface.h"
#include "BallType.h"
#include "BaseGameMode.generated.h"

class UGameWidget;
class IAGInterstitialAdInterface;
class USecondChanceWidget;
class IAGRewardedAdInterface;

UCLASS()
class FLAPPYHOOP_API ABaseGameMode : public AGameMode, public IGameModeInterface
{
	GENERATED_BODY()

public:

	virtual FOnGameStarted& OnGameStartedDelegate() override { return OnGameStarted; }

	virtual FOnViewportFetched& OnViewportFetchedDelegate() { return OnViewportFetched; }

	virtual FOnGameReset& OnGameResetDelegate() { return OnGameReset; }

	virtual void ResetGame() override;

	virtual float GetMaxGameTime() const { return MaxGameTime; }

	virtual void SetNewGameTime();

	virtual FOnPointScored& OnPointScoredDelegate() override { return OnPointScored; }

	virtual void UpdateScore() override;

	virtual void UpdateScoreMultiplier() override { ScoreMultiplier++; }

	virtual int GetScoreMultiplier() const override { return ScoreMultiplier; }

	virtual void ResetScoreMultiplier() override { ScoreMultiplier = 1; }

	virtual int GetCurrentScore() override { return CurrentScore; }

	virtual FOnTimeEnded& OnTimeEndedDelegate() override { return OnTimeEnded; }

	virtual bool GetTimeEndedBool() const override { return bTimeEnded; }
	virtual void SetTimeEndedBool(bool NewTimeEnded) override { bTimeEnded = NewTimeEnded; }

	virtual void EndGame() override;

	virtual int GetHighScore() const override { return HighScore; }

	virtual void ApplyBallSettings() override;

	virtual void SetBallType(EBallType NewBallType) override { BallType = NewBallType; }

	virtual EBallType GetBallType() const override { return BallType; }

	virtual FOnCoinCollected& OnCoinCollectedDelegate() override { return OnCoinCollected; }

	virtual int32 GetTotalCoins() const override;

	virtual int32 GetTotalGems() const override;

	virtual int32 GetCollectedGems() const { return CollectedGems; }

	virtual void AddCoin() override;

	virtual void AddGem() override;

	virtual int32 GetCollectedCoins() const override { return CollectedCoins; }

	virtual void ActivateCoin() override;

	virtual IGameInstanceInterface* GetGameInstanceInterface() const override { return GameInstanceInterface; }

	virtual  void LoadRewardedAd() override;

	virtual FOnSecondChanceGranted& OnSecondChanceGrantedDelegate() override { return OnSecondChanceGranted; }

	virtual void CreateSecondChanceWidget() override;

	UFUNCTION()
	virtual void GrantSecondChance(FRewardItem Reward) override;

	virtual bool GetCanWatchAd() const override { return bCanWatchAd; }

	virtual void SetCanWatchAd(bool bNewCanWatchAd) override { bCanWatchAd = bNewCanWatchAd; }

	virtual int32 GetGemsNeededForSecondChance() const override { return GemsNeededForSecondChance; }

	virtual void IncrementRetryCount() override { RetryCount++; }

	virtual void IncrementGemsNeededForSecondChance() override;

	virtual int32 GetGemsSpent() const override { return GemsSpent; }

	virtual void IncrementGemsSpentCount(int32 IncrementBy) override { GemsSpent += IncrementBy; }

	FTimerHandle& GetSwitchSidesTimerHandle() override { return SwitchSidesTimerHandle; }

	virtual void StopInterstitialTimer() override;

protected:

	virtual void BeginPlay() override;

private:

	virtual FVector2D GetViewportSize() const override { return ViewportSize; }

	UGameWidget* GameWidgetInstance = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameWidget> GameWidgetClass;

	FOnGameStarted OnGameStarted;

	FOnViewportFetched OnViewportFetched;

	FOnGameReset OnGameReset;

	void FetchViewportSize();

	FVector2D ViewportSize;

	UWorld* World = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Parameters")
	float GameTimeReductionRate = 0.05f;

	UPROPERTY(EditDefaultsOnly, Category = "Parameters")
	float MaxGameTime = 10.0f;

	float MaxGameTimeOriginal;

	FOnPointScored OnPointScored;

	int CurrentScore = 0;
	int ScoreMultiplier = 1;

	FOnTimeEnded OnTimeEnded;

	bool bTimeEnded = false;

	void EndTime();

	IGameInstanceInterface* GameInstanceInterface = nullptr;

	int HighScore = 0;

	UPROPERTY(EditDefaultsOnly)
	TMap<EBallType, UStaticMesh*> BallMeshes;

	class ABall* DefaultBall;
	class ACoins* DefaultCoin;
	class AGems* DefaultGem;

	UPROPERTY(EditDefaultsOnly)
	EBallType BallType;

	FOnCoinCollected OnCoinCollected;

	int CollectedCoins = 0;
	int CollectedGems = 0;

	UPROPERTY(EditDefaultsOnly, Category= "Collectibles", meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0"))
	float CollectibleChancePercent = 25.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Collectibles", meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0"))
	float CoinChancePercent = 75.0f;

	UPROPERTY(VisibleDefaultsOnly, Category = "Collectibles")
	float GemChancePercent = 25.0f;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	TScriptInterface<IAGInterstitialAdInterface> InterstitialAdInterface;

	TScriptInterface< IAGRewardedAdInterface> RewardedAdInterface;

	void LoadInterstitialAd();

	UFUNCTION()
	void ShowInterstitialAdIfAvailable();

	UFUNCTION()
	void ShowRewardedAdIfAvailable();

	FTimerHandle InterstitialAdTimer;

	FOnSecondChanceGranted OnSecondChanceGranted;

	UPROPERTY()
	USecondChanceWidget* SecondChanceWidgetInstance;

	UPROPERTY(EditDefaultsOnly, Category = "Second Chance")
	bool bCanWatchAd = true;


	UPROPERTY(EditDefaultsOnly, Category = "Second Chance")
	int32 BaseGemCost = 1;

	int32 GemsNeededForSecondChance;

	int32 RetryCount = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Second Chance")
	float GrowthRate = 1.5f;

	int32 GemsSpent = 0;

	FTimerHandle SwitchSidesTimerHandle;

	void StartInterstitialTimer();

	bool CanShowInterstitialAd() const;
};
