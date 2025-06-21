// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FunctionsLibrary.generated.h"

class IGameInstanceInterface;
class IGameModeInterface;

UCLASS()
class FLAPPYHOOP_API UFunctionsLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:

	static IGameInstanceInterface* GetGameInstanceInterface(UObject* WorldContextObject);

	static IGameModeInterface* GetGameModeInterface(UObject* WorldContextObject);
};
