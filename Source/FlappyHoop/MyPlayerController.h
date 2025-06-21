// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MyPlayerController.generated.h"

UCLASS()
class FLAPPYHOOP_API AMyPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:

	AMyPlayerController();

private:

	virtual void BeginPlay() override;

	virtual void SetupInputComponent() override;

	UFUNCTION()
	void HandleTouchPressed(ETouchIndex::Type FingerIndex, FVector Location);

	class ABall* Ball = nullptr;

	void EnableControllerInput();
	void DisableControllerInput();

};
