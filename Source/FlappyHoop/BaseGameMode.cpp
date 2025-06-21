// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseGameMode.h"
#include "Kismet/Gameplaystatics.h"
#include "GameWidget.h"

void ABaseGameMode::BeginPlay()
{
	Super::BeginPlay();

	World = GetWorld();
	FetchViewportSize();

	GameWidgetInstance = CreateWidget<UGameWidget>(World, GameWidgetClass);

	if (GameWidgetInstance)
	{
		GameWidgetInstance->AddToViewport();
	}
}

void ABaseGameMode::FetchViewportSize()
{
	APlayerController* PlayerController = World->GetFirstPlayerController();

	int32 SizeX = 0;
	int32 SizeY = 0;

	PlayerController->GetViewportSize(SizeX, SizeY);

	if (SizeX == 0 || SizeY == 0)
	{
		FTimerHandle RetryHandle;
		GetWorld()->GetTimerManager().SetTimer(
			RetryHandle,
			this,
			&ABaseGameMode::FetchViewportSize,
			0.1f,
			false
		);
		return;
	}

	ViewportSize = FVector2D(SizeX, SizeY);
	OnViewportFetched.Broadcast();
}