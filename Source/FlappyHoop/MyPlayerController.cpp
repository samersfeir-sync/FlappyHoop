// Fill out your copyright notice in the Description page of Project Settings.


#include "MyPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Ball.h"

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