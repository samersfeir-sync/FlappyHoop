// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "GameModeInterface.h"
#include "BallType.h"
#include "BaseGameMode.generated.h"

class UGameWidget;

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

	virtual void AddCoin() override;

	virtual int32 GetCollectedCoins() const override { return CollectedCoins; }

	virtual void ActivateCoin() override;

	virtual IGameInstanceInterface* GetGameInstanceInterface() const override { return GameInstanceInterface; }

protected:

	virtual void BeginPlay() override;

	virtual FVector2D GetViewportSize() const override { return ViewportSize; }

private:

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

	UPROPERTY(EditDefaultsOnly)
	EBallType BallType;

	FOnCoinCollected OnCoinCollected;

	int CollectedCoins = 0;

	UPROPERTY(EditDefaultsOnly, Category= "Parameters", meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0"))
	float CoinChancePercent = 25.0f;
};
