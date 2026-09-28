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
	
protected:

	virtual void BeginPlay() override;

private:

	void SetAspectRatio();

	UPROPERTY(Transient)
	TObjectPtr<class ABaseGameMode> GameMode = nullptr;
};
