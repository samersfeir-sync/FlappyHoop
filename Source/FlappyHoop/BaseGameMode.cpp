// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseGameMode.h"
#include "Kismet/Gameplaystatics.h"
#include "FunctionsLibrary.h"
#include "GameWidget.h"
#include "UserProgression.h"
#include "GameInstanceInterface.h"
#include "Ball.h"
#include "Coins.h"
#include "Gems.h"
#include "Ads/AGAdLibrary.h"
#include "Interface/AGInterstitialAdInterface.h"
#include "Interface/AGRewardedAdInterface.h"
#include "SecondChanceWidget.h"

void ABaseGameMode::ResetGame()
{
	UGameplayStatics::SetGamePaused(World, false);
	OnGameReset.Broadcast();
	MaxGameTime = MaxGameTimeOriginal;
	CurrentScore = 0;
	ScoreMultiplier = 1;
	bTimeEnded = false;
	bCanWatchAd = true;
	CollectedCoins = 0;
	CollectedGems = 0;
	GemsSpent = 0;
	RetryCount = 0;
	GemsNeededForSecondChance = BaseGemCost;
	LoadInterstitialAd();
	World->GetTimerManager().SetTimer(InterstitialAdTimer, this, &ABaseGameMode::LoadInterstitialAd, 15.0f, true);
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

int32 ABaseGameMode::GetTotalGems() const
{
	return GameInstanceInterface->GetUserProgression().TotalGems;
}

void ABaseGameMode::AddCoin()
{
	CollectedCoins++;
}

void ABaseGameMode::AddGem()
{
	CollectedGems++;
	FUserProgression UseProgression = GameInstanceInterface->GetUserProgression();
	UseProgression.TotalGems += 1;
	GameInstanceInterface->SaveUserProgression(UseProgression);
}

void ABaseGameMode::ActivateCoin()
{
	if (DefaultCoin->IsCoinActive() || DefaultGem->IsCoinActive())
		return;

	float Roll = FMath::FRandRange(0.0f, 100.0f);

	if (Roll <= CollectibleChancePercent)
	{
		(FMath::FRandRange(0.0f, 100.0f) <= CoinChancePercent ? DefaultCoin : DefaultGem)->ActivateCoin(true);
	}
}

void ABaseGameMode::LoadRewardedAd()
{
	RewardedAdInterface = UAGAdLibrary::MakeRewardedAd(
		GameInstanceInterface->GetRewardedAdUnitID());

	if (RewardedAdInterface)
	{
		RewardedAdInterface->LoadAd();

		FOnRewardedAdLoadedDelegate Delegate;
		Delegate.BindDynamic(this, &ABaseGameMode::ShowRewardedAdIfAvailable);
		RewardedAdInterface->BindEventToOnAdLoaded(Delegate);

		FOnRewardedAdFailedToLoadDelegate FailedToLoadDelegate;
		FailedToLoadDelegate.BindDynamic(SecondChanceWidgetInstance, &USecondChanceWidget::RewardAdFailed);
		RewardedAdInterface->BindEventToOnAdFailedToLoad(FailedToLoadDelegate);

		FOnRewardedAdFailedToShowDelegate FailedToShowDelegate;
		FailedToShowDelegate.BindDynamic(SecondChanceWidgetInstance, &USecondChanceWidget::RewardAdFailed);
		RewardedAdInterface->BindEventToOnAdFailedToShow(FailedToShowDelegate);
	}
}

void ABaseGameMode::CreateSecondChanceWidget()
{
	if (SecondChanceWidgetInstance)
	{
		GameWidgetInstance->ShowSecondChanceWidget(true);
	}
}

void ABaseGameMode::IncrementGemsNeededForSecondChance()
{
	GemsNeededForSecondChance = FMath::CeilToInt(BaseGemCost * FMath::Pow(GrowthRate, RetryCount));
}

void ABaseGameMode::BeginPlay()
{
	Super::BeginPlay();

	World = GetWorld();
	FetchViewportSize();
	GemsNeededForSecondChance = BaseGemCost;

	DefaultBall = Cast<ABall>(UGameplayStatics::GetActorOfClass(World, ABall::StaticClass()));
	DefaultCoin = Cast<ACoins>(UGameplayStatics::GetActorOfClass(World, ACoins::StaticClass()));
	DefaultGem = Cast<AGems>(UGameplayStatics::GetActorOfClass(World, AGems::StaticClass()));

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
		SecondChanceWidgetInstance = GameWidgetInstance->SecondChanceWidget;
	}

	OnTimeEnded.AddUObject(this, &ABaseGameMode::EndTime);

	//cache variables that are set from the editor
	MaxGameTimeOriginal = MaxGameTime;

	World->GetTimerManager().SetTimer(InterstitialAdTimer, this, &ABaseGameMode::LoadInterstitialAd, 15.0f, true);

	OnGameStarted.AddUObject(this, &ABaseGameMode::StopInterstitialTimer);
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

void ABaseGameMode::ShowInterstitialAdIfAvailable()
{
	if (GameWidgetInstance)
	{
		EWidgetState CurrentState = GameWidgetInstance->GetCurrentWidgetState();

		switch (CurrentState)
		{
		case EWidgetState::Playing:
			GameWidgetInstance->PauseGame();
			break;

		default:
			break;
		}
	}

	InterstitialAdInterface->Show();
}

void ABaseGameMode::ShowRewardedAdIfAvailable()
{
	FOnRewardedAdUserEarhedRewardDelegate Delegate;
	Delegate.BindDynamic(this, &ABaseGameMode::GrantSecondChance);
	RewardedAdInterface->BindEventToOnUserEarnedReward(Delegate);
	RewardedAdInterface->Show();
	GameWidgetInstance->ShowSecondChanceWidget(false);
	bCanWatchAd = false;
}

void ABaseGameMode::GrantSecondChance(FRewardItem Reward)
{
	bTimeEnded = false;
	//MaxGameTime = MaxGameTimeOriginal;
	OnSecondChanceGranted.Broadcast();
}

void ABaseGameMode::LoadInterstitialAd()
{
	InterstitialAdInterface = UAGAdLibrary::MakeInterstitialAd(
		GameInstanceInterface->GetInterstitialAdUnitID());

	if (InterstitialAdInterface)
	{	
		InterstitialAdInterface->LoadAd();

		FOnInterstitialAdLoadedDelegate Delegate;
		Delegate.BindDynamic(this, &ABaseGameMode::ShowInterstitialAdIfAvailable);
		InterstitialAdInterface->BindEventToOnAdLoaded(Delegate);
	}
}

void ABaseGameMode::StopInterstitialTimer()
{
	GetWorld()->GetTimerManager().ClearTimer(InterstitialAdTimer);
}

#if WITH_EDITOR
void ABaseGameMode::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FName PropertyName = (PropertyChangedEvent.Property != nullptr)
		? PropertyChangedEvent.Property->GetFName()
		: NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(ABaseGameMode, CoinChancePercent))
	{
		CoinChancePercent = FMath::Clamp(CoinChancePercent, 0.0f, 100.0f);
		GemChancePercent = 100.0f - CoinChancePercent;
	}
}
#endif