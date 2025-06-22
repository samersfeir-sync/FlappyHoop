// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraActor.h"
#include "CameraMain.generated.h"

/**
 * 
 */
UCLASS()
class FLAPPYHOOP_API ACameraMain : public ACameraActor
{
	GENERATED_BODY()
	
private:

	virtual void BeginPlay() override;

	void SetAspectRatio();

	class IGameModeInterface* GameModeInterface = nullptr;
};
