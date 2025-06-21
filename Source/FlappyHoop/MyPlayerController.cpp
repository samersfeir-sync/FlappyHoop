// Fill out your copyright notice in the Description page of Project Settings.


#include "MyPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Ball.h"
#include "FunctionsLibrary.h"
#include "GameModeInterface.h"

AMyPlayerController::AMyPlayerController()
{
	bEnableTouchEvents = true;
	bShowMouseCursor = true;
    DisableInput(this);
}

void AMyPlayerController::BeginPlay()
{
    Super::BeginPlay();

    Ball = Cast<ABall>(UGameplayStatics::GetActorOfClass(GetWorld(), ABall::StaticClass()));

    if (IGameModeInterface* GameModeInterface = UFunctionsLibrary::GetGameModeInterface(this))
    {
        GameModeInterface->OnGameStartedDelegate().AddUObject(this, &AMyPlayerController::EnableControllerInput);
        GameModeInterface->OnGameResetDelegate().AddUObject(this, &AMyPlayerController::DisableControllerInput);
    }
}

void AMyPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (InputComponent)
    {
        InputComponent->BindTouch(IE_Pressed, this, &AMyPlayerController::HandleTouchPressed);
    }
}

void AMyPlayerController::HandleTouchPressed(ETouchIndex::Type FingerIndex, FVector Location)
{
    if (Ball)
    {
        Ball->LaunchBall();
    }
}

void AMyPlayerController::EnableControllerInput()
{
    EnableInput(this);
}

void AMyPlayerController::DisableControllerInput()
{
    DisableInput(this);
}
