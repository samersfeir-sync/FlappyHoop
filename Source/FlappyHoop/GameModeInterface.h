// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameModeInterface.generated.h"

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
};
