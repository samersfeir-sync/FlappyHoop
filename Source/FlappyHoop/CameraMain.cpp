// Fill out your copyright notice in the Description page of Project Settings.


#include "CameraMain.h"
#include "GameModeInterface.h"
#include "FunctionsLibrary.h"
#include "Camera/CameraComponent.h"

void ACameraMain::BeginPlay()
{
	Super::BeginPlay();

	GameModeInterface = UFunctionsLibrary::GetGameModeInterface(this);

	if (GameModeInterface)
	{
		GameModeInterface->OnViewportFetchedDelegate().AddUObject(this, &ACameraMain::SetAspectRatio);
	}
}

void ACameraMain::SetAspectRatio()
{
	FVector2D ViewportSize = GameModeInterface->GetViewportSize();

	float AspectRatio = ViewportSize.X / ViewportSize.Y;

	GetCameraComponent()->SetAspectRatio(AspectRatio);
}
