// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "BallInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UBallInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class FLAPPYHOOP_API IBallInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	virtual FORCEINLINE FVector GetBallVelocity() const = 0;
	virtual FORCEINLINE void ChangeBallDirection() = 0;
};
