// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "UserProgression.h"
#include "MySaveGame.generated.h"

/**
 * 
 */
UCLASS()
class FLAPPYHOOP_API UMySaveGame : public USaveGame
{
	GENERATED_BODY()
	
public:

	UPROPERTY(EditDefaultsOnly, SaveGame, Category = "User Progression")
	FUserProgression UserProgression;
};
