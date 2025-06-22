// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "GameModeInterface.h"
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

	virtual FORCEINLINE float GetMaxGameTime() const { return MaxGameTime; }

	virtual FORCEINLINE void SetNewGameTime();

	virtual FOnPointScored& OnPointScoredDelegate() override { return OnPointScored; }

	virtual FORCEINLINE void UpdateScore() override;

	virtual FORCEINLINE int GetCurrentScore() override { return CurrentScore; }

	virtual FOnTimeEnded& OnTimeEndedDelegate() override { return OnTimeEnded; }

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
};
