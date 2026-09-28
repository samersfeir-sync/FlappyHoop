// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MyPlayerController.generated.h"

class ABall;

/** Tap (or Space on desktop) launches the ball whenever the game mode allows it. Back goes to the UI. */
UCLASS()
class FLAPPYHOOP_API AMyPlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	AMyPlayerController();

protected:

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:

	void HandleTouchPressed(ETouchIndex::Type FingerIndex, FVector Location);

	/** Desktop/editor testing: Space launches the ball like a tap. */
	void HandleLaunchKey();

	void Launch();

	/** Android Back (Backspace on desktop): handed to the UI root. Works while paused. */
	void HandleBackKey();

	UPROPERTY(Transient)
	TObjectPtr<ABall> Ball = nullptr;
};
