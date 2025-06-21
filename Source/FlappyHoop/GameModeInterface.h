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

class FLAPPYHOOP_API IGameModeInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	virtual FOnGameStarted& OnGameStartedDelegate() = 0;
	virtual FOnViewportFetched& OnFOnViewportFetchedDelegate() = 0;
	virtual FVector2D GetViewportSize() const = 0;
};
