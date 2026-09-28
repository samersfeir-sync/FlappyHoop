// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/MyPlayerController.h"
#include "Framework/BaseGameMode.h"
#include "Gameplay/Ball.h"
#include "UI/HUD/GameWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Components/InputComponent.h"
#include "InputCoreTypes.h"

AMyPlayerController::AMyPlayerController()
{
	bEnableTouchEvents = true;
	bShowMouseCursor = true;
}

void AMyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	Ball = Cast<ABall>(UGameplayStatics::GetActorOfClass(GetWorld(), ABall::StaticClass()));
}

void AMyPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!InputComponent)
	{
		return;
	}

	InputComponent->BindTouch(IE_Pressed, this, &AMyPlayerController::HandleTouchPressed);

	// Back must work while paused (it's how you resume or leave the pause menu).
	InputComponent->BindKey(EKeys::Android_Back, IE_Pressed, this, &AMyPlayerController::HandleBackKey).bExecuteWhenPaused = true;

#if !PLATFORM_ANDROID && !PLATFORM_IOS
	// Not bound to the mouse: with "Use Mouse For Touch" enabled a click would fire twice.
	InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &AMyPlayerController::HandleLaunchKey);
	InputComponent->BindKey(EKeys::BackSpace, IE_Pressed, this, &AMyPlayerController::HandleBackKey).bExecuteWhenPaused = true;
#endif
}

void AMyPlayerController::HandleTouchPressed(ETouchIndex::Type FingerIndex, FVector Location)
{
	Launch();
}

void AMyPlayerController::HandleLaunchKey()
{
	Launch();
}

void AMyPlayerController::Launch()
{
	// The game mode is the single source of truth (playing, clock not expired, no basket being celebrated).
	const ABaseGameMode* GameMode = ABaseGameMode::Get(this);
	if (Ball && GameMode && GameMode->CanLaunchBall())
	{
		Ball->LaunchBall();
	}
}

void AMyPlayerController::HandleBackKey()
{
	const ABaseGameMode* GameMode = ABaseGameMode::Get(this);
	if (UGameWidget* UI = GameMode ? GameMode->GetGameWidget() : nullptr)
	{
		UI->HandleBackButton();
	}
}
