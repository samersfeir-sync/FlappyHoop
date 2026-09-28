// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/CameraMain.h"
#include "Framework/BaseGameMode.h"
#include "Camera/CameraComponent.h"

void ACameraMain::BeginPlay()
{
	Super::BeginPlay();

	GameMode = ABaseGameMode::Get(this);

	if (GameMode)
	{
		GameMode->OnViewportFetched.AddUObject(this, &ACameraMain::SetAspectRatio);
	}
}

void ACameraMain::SetAspectRatio()
{
	if (!GameMode)
	{
		return;
	}

	const FVector2D ViewportSize = GameMode->GetViewportSize();
	if (ViewportSize.X <= 0.0 || ViewportSize.Y <= 0.0)
	{
		return;
	}

	GetCameraComponent()->SetAspectRatio(static_cast<float>(ViewportSize.X / ViewportSize.Y));
}
