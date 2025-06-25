// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseGameMode.h"
#include "Kismet/Gameplaystatics.h"
#include "FunctionsLibrary.h"
#include "GameWidget.h"
#include "UserProgression.h"
#include "GameInstanceInterface.h"

void ABaseGameMode::ResetGame()
{
	UGameplayStatics::SetGamePaused(World, false);
	OnGameReset.Broadcast();
	MaxGameTime = MaxGameTimeOriginal;
	CurrentScore = 0;
	ScoreMultiplier = 1;
	bTimeEnded = false;
}

void ABaseGameMode::SetNewGameTime()
{
	const float Reduction = MaxGameTimeOriginal * GameTimeReductionRate;
	MaxGameTime -= Reduction;
	MaxGameTime = FMath::Max(MaxGameTime, 3.0f);
}

void ABaseGameMode::UpdateScore()
{
	CurrentScore += ScoreMultiplier;
}

void ABaseGameMode::EndGame()
{
	GameWidgetInstance->ShowGameOverWidget(true);
	GameWidgetInstance->EndComboTimer();

	FUserProgression UserProgression = GameInstanceInterface->GetUserProgression();
	if (CurrentScore > HighScore)
	{
		HighScore = CurrentScore;
		UserProgression.HighScore = CurrentScore;
		GameInstanceInterface->SaveUserProgression(UserProgression);
	}
}

void ABaseGameMode::BeginPlay()
{
	Super::BeginPlay();

	World = GetWorld();
	FetchViewportSize();

	GameInstanceInterface = UFunctionsLibrary::GetGameInstanceInterface(World);

	if (GameInstanceInterface)
	{
		HighScore = GameInstanceInterface->GetUserProgression().HighScore;
	}

	GameWidgetInstance = CreateWidget<UGameWidget>(World, GameWidgetClass);

	if (GameWidgetInstance)
	{
		GameWidgetInstance->SetWorldReference(World);
		GameWidgetInstance->AddToViewport();
	}

	OnTimeEnded.AddUObject(this, &ABaseGameMode::EndTime);

	//cache variables that are set from the editor
	MaxGameTimeOriginal = MaxGameTime;
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

void ABaseGameMode::EndTime()
{
	SetTimeEndedBool(true);
}
