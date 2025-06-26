// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseGameMode.h"
#include "Kismet/Gameplaystatics.h"
#include "FunctionsLibrary.h"
#include "GameWidget.h"
#include "UserProgression.h"
#include "GameInstanceInterface.h"
#include "Ball.h"
#include "Coins.h"

void ABaseGameMode::ResetGame()
{
	UGameplayStatics::SetGamePaused(World, false);
	OnGameReset.Broadcast();
	MaxGameTime = MaxGameTimeOriginal;
	CurrentScore = 0;
	ScoreMultiplier = 1;
	bTimeEnded = false;
	CollectedCoins = 0;
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
	GameWidgetInstance->EndComboTimer();

	FUserProgression UserProgression = GameInstanceInterface->GetUserProgression();
	if (CurrentScore > HighScore)
	{
		HighScore = CurrentScore;
		UserProgression.HighScore = CurrentScore;
	}

	UserProgression.TotalCoins += CollectedCoins;
	GameInstanceInterface->SaveUserProgression(UserProgression);
	GameWidgetInstance->ShowGameOverWidget(true);
}

void ABaseGameMode::ApplyBallSettings()
{
	if (UStaticMesh** FoundMesh = BallMeshes.Find(BallType))
	{
		DefaultBall->BallMesh->SetStaticMesh(*FoundMesh);
	}
}

int32 ABaseGameMode::GetTotalCoins() const
{
	return GameInstanceInterface->GetUserProgression().TotalCoins;
}

void ABaseGameMode::AddCoin()
{
	CollectedCoins++;
}

void ABaseGameMode::ActivateCoin()
{
	if (DefaultCoin->IsCoinActive())
		return;

	float Roll = FMath::FRandRange(0.0f, 100.0f);

	if (Roll <= ChancePercent)
	{
		DefaultCoin->ActivateCoin(true);
	}
}

void ABaseGameMode::BeginPlay()
{
	Super::BeginPlay();

	World = GetWorld();
	FetchViewportSize();

	DefaultBall = Cast<ABall>(UGameplayStatics::GetActorOfClass(World, ABall::StaticClass()));
	DefaultCoin = Cast<ACoins>(UGameplayStatics::GetActorOfClass(World, ACoins::StaticClass()));

	GameInstanceInterface = UFunctionsLibrary::GetGameInstanceInterface(World);

	if (GameInstanceInterface)
	{
		HighScore = GameInstanceInterface->GetUserProgression().HighScore;
		BallType = GameInstanceInterface->GetUserProgression().BallType;

		if (DefaultBall)
		{
			ApplyBallSettings();
		}
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
