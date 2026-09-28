// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/BaseGameMode.h"
#include "Core/FlappyHoop.h"
#include "Data/UserProgression.h"
#include "Gameplay/Ball.h"
#include "Gameplay/Coins.h"
#include "Gameplay/Gems.h"
#include "Systems/EconomySubsystem.h"
#include "Systems/SaveSubsystem.h"
#include "Systems/AdsSubsystem.h"
#include "Systems/BallCatalogSubsystem.h"
#include "UI/HUD/GameWidget.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

UGameWidget* ABaseGameMode::GetGameWidget() const
{
	return GameWidgetInstance;
}

ABaseGameMode* ABaseGameMode::Get(const UObject* WorldContextObject)
{
	// ReturnNull: a missing world (e.g. the widget designer) must not assert.
	const UWorld* World = (WorldContextObject && GEngine)
		? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull)
		: nullptr;
	return World ? Cast<ABaseGameMode>(World->GetAuthGameMode()) : nullptr;
}

// ---------------------------------------------------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------------------------------------------------

void ABaseGameMode::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();

	bCanWatchAdDefault = bCanWatchAd;
	GemsNeededForSecondChance = BaseGemCost;
	CurrentRoundLength = MaxGameTime;
	RoundTimeRemaining = MaxGameTime;

	DefaultBall = Cast<ABall>(UGameplayStatics::GetActorOfClass(World, ABall::StaticClass()));

	// AGems derives from ACoins, so pick the exact classes explicitly.
	for (TActorIterator<ACoins> It(World); It; ++It)
	{
		if (AGems* Gem = Cast<AGems>(*It))
		{
			if (!DefaultGem) { DefaultGem = Gem; }
		}
		else if (!DefaultCoin)
		{
			DefaultCoin = *It;
		}
	}

	if (const USaveSubsystem* Save = USaveSubsystem::Get(this))
	{
		const FUserProgression& Progression = Save->GetProgression();
		HighScore = Progression.HighScore;
	}

	ApplyBallSettings();
	if (UBallCatalogSubsystem* Catalog = UBallCatalogSubsystem::Get(this))
	{
		Catalog->OnBallsChanged.AddUObject(this, &ABaseGameMode::ApplyBallSettings);
	}

	GameWidgetInstance = CreateWidget<UGameWidget>(World, GameWidgetClass);
	if (GameWidgetInstance)
	{
		GameWidgetInstance->AddToViewport();
	}

	if (UAdsSubsystem* Ads = UAdsSubsystem::Get(this))
	{
		Ads->ConfigureInterstitials(InterstitialEveryNGames, InterstitialCooldownSeconds, InterstitialRetrySeconds);
		Ads->PreloadInterstitial();
	}
}

void ABaseGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelRewardedAd();

	if (UBallCatalogSubsystem* Catalog = UBallCatalogSubsystem::Get(this))
	{
		Catalog->OnBallsChanged.RemoveAll(this);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}

	Super::EndPlay(EndPlayReason);
}

void ABaseGameMode::StartRoundTimer()
{
	GetWorldTimerManager().SetTimer(RoundTimer, this, &ABaseGameMode::RoundTimerStepElapsed, RoundTimerStep, true);
}

void ABaseGameMode::StopRoundTimer()
{
	GetWorldTimerManager().ClearTimer(RoundTimer);
}

void ABaseGameMode::RefillRoundTime()
{
	RoundTimeRemaining = CurrentRoundLength;
	OnRoundTimeUpdated.Broadcast(1.0f);
}

void ABaseGameMode::RoundTimerStepElapsed()
{
	// The clock only runs while actually playing, and freezes during a basket celebration.
	if (CurrentGameState != EGameStateEnum::Playing || bTimeEnded || bBasketPending)
	{
		return;
	}

	RoundTimeRemaining = FMath::Max(0.0f, RoundTimeRemaining - RoundTimerStep);
	OnRoundTimeUpdated.Broadcast(GetRoundTimeFraction());

	if (RoundTimeRemaining <= 0.0f)
	{
		bTimeEnded = true;
		OnTimeEnded.Broadcast();

		GetWorldTimerManager().SetTimer(TimeUpFallbackTimer, this, &ABaseGameMode::OnTimeUpFallback, TimeUpLandingFallbackSeconds, false);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Run phases
// ---------------------------------------------------------------------------------------------------------------------

void ABaseGameMode::SetRunPhase(EGameStateEnum NewPhase)
{
	if (NewPhase == CurrentGameState)
	{
		return;
	}

	const EGameStateEnum OldPhase = CurrentGameState;
	CurrentGameState = NewPhase;

	UE_LOG(LogFlappyHoop, Verbose, TEXT("Run phase %d -> %d"), static_cast<int32>(OldPhase), static_cast<int32>(NewPhase));
	OnRunPhaseChanged.Broadcast(OldPhase, NewPhase);
}

void ABaseGameMode::ResetRunState()
{
	StopRoundTimer();
	GetWorldTimerManager().ClearTimer(TimeUpFallbackTimer);

	CurrentRoundLength = MaxGameTime;
	RoundTimeRemaining = MaxGameTime;
	bTimeEnded = false;
	bBasketPending = false;

	CurrentScore = 0;
	ScoreMultiplier = 1;
	bLastBasketSwish = false;

	CollectedCoins = 0;
	CollectedGems = 0;
	GemsSpent = 0;

	bCanWatchAd = bCanWatchAdDefault;
	RetryCount = 0;
	GemsNeededForSecondChance = BaseGemCost;
}

void ABaseGameMode::StartRun()
{
	if (CurrentGameState != EGameStateEnum::MainMenu)
	{
		return;
	}

	ResetRunState();
	SetRunPhase(EGameStateEnum::Playing);
	OnRoundTimeUpdated.Broadcast(1.0f);
	StartRoundTimer();
	OnGameStarted.Broadcast();
}

void ABaseGameMode::ReturnToMenu()
{
	UGameplayStatics::SetGamePaused(this, false);

	// Drop anything still in flight from the last run so it can't act on the menu.
	CancelRewardedAd();

	ResetRunState();
	OnGameReset.Broadcast();
	SetRunPhase(EGameStateEnum::MainMenu);

	if (UAdsSubsystem* Ads = UAdsSubsystem::Get(this))
	{
		Ads->TryShowInterstitial();
	}
}

void ABaseGameMode::EndGame()
{
	if (CurrentGameState != EGameStateEnum::Playing && CurrentGameState != EGameStateEnum::SecondChance)
	{
		return;
	}

	CancelRewardedAd();

	if (UEconomySubsystem* Economy = UEconomySubsystem::Get(this))
	{
		Economy->AddCoins(CollectedCoins); // gems are banked the moment they're picked up
	}

	USaveSubsystem* Save = USaveSubsystem::Get(this);
	if (Save && CurrentScore > HighScore)
	{
		HighScore = CurrentScore;

		FUserProgression Progression = Save->GetProgression();
		Progression.HighScore = HighScore;
		Save->Save(Progression);
	}

	StopRoundTimer();
	if (UAdsSubsystem* Ads = UAdsSubsystem::Get(this))
	{
		Ads->NotifyRunFinished();
	}
	SetRunPhase(EGameStateEnum::GameOver);
}

// ---------------------------------------------------------------------------------------------------------------------
// Round timer / baskets / landing
// ---------------------------------------------------------------------------------------------------------------------

float ABaseGameMode::GetRoundTimeFraction() const
{
	return CurrentRoundLength > 0.0f ? FMath::Clamp(RoundTimeRemaining / CurrentRoundLength, 0.0f, 1.0f) : 0.0f;
}

void ABaseGameMode::OnTimeUpFallback()
{
	// A basket in flight gets its chance; otherwise treat it as landed (fixes the "ball already resting" soft-lock).
	if (bBasketPending)
	{
		GetWorldTimerManager().SetTimer(TimeUpFallbackTimer, this, &ABaseGameMode::OnTimeUpFallback, 1.0f, false);
		return;
	}

	NotifyBallLanded();
}

void ABaseGameMode::ShortenRound()
{
	CurrentRoundLength = FMath::Max(CurrentRoundLength - MaxGameTime * GameTimeReductionRate, MinGameTime);
}

bool ABaseGameMode::CanLaunchBall() const
{
	return CurrentGameState == EGameStateEnum::Playing && !bTimeEnded && !bBasketPending;
}

void ABaseGameMode::NotifyBasketPending()
{
	if (CurrentGameState == EGameStateEnum::Playing)
	{
		bBasketPending = true;
	}
}

void ABaseGameMode::ScoreBasket(bool bSwish)
{
	if (CurrentGameState != EGameStateEnum::Playing || !bBasketPending)
	{
		return;
	}

	bBasketPending = false;

	// A basket that lands after the clock hit zero still counts, and saves the run.
	bTimeEnded = false;
	GetWorldTimerManager().ClearTimer(TimeUpFallbackTimer);
	ShortenRound();
	RefillRoundTime();

	bLastBasketSwish = bSwish;
	CurrentScore += ScoreMultiplier * (bSwish ? SwishScoreMultiplier : 1);

	// Listeners read the multiplier used for this basket, then the combo grows.
	OnPointScored.Broadcast();
	ScoreMultiplier++;

	MaybeSpawnCollectible();
}

void ABaseGameMode::NotifyBallLanded()
{
	// Landing only matters once the clock ran out (and not while a basket is being celebrated).
	if (CurrentGameState != EGameStateEnum::Playing || !bTimeEnded || bBasketPending)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(TimeUpFallbackTimer);

	const UEconomySubsystem* Economy = UEconomySubsystem::Get(this);
	const bool bCanAffordGems = Economy && Economy->GetGems() >= GemsNeededForSecondChance;

	if (bCanWatchAd || bCanAffordGems)
	{
		SetRunPhase(EGameStateEnum::SecondChance);
	}
	else
	{
		EndGame();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Ball / collectibles
// ---------------------------------------------------------------------------------------------------------------------

void ABaseGameMode::ApplyBallSettings()
{
	if (!DefaultBall || !DefaultBall->BallMesh)
	{
		return;
	}

	const UBallCatalogSubsystem* Catalog = UBallCatalogSubsystem::Get(this);
	const EBallType Selected = Catalog ? Catalog->GetSelected() : EBallType::Basketball;

	UStaticMesh* Mesh = nullptr;
	if (const FBallCatalogEntry* Entry = Catalog ? Catalog->Find(Selected) : nullptr)
	{
		Mesh = Entry->Mesh;
	}

	if (!Mesh)
	{
		if (const TObjectPtr<UStaticMesh>* LegacyMesh = BallMeshes.Find(Selected))
		{
			Mesh = *LegacyMesh;
		}
	}

	if (Mesh)
	{
		DefaultBall->BallMesh->SetStaticMesh(Mesh);
	}
}

void ABaseGameMode::AddGem()
{
	CollectedGems++;

	// Premium currency: banked immediately.
	if (UEconomySubsystem* Economy = UEconomySubsystem::Get(this))
	{
		Economy->AddGems(1);
	}
}

void ABaseGameMode::MaybeSpawnCollectible()
{
	if (!DefaultCoin || !DefaultGem || DefaultCoin->IsCoinActive() || DefaultGem->IsCoinActive())
	{
		return;
	}

	if (FMath::FRandRange(0.0f, 100.0f) <= CollectibleChancePercent)
	{
		ACoins* ToActivate = FMath::FRandRange(0.0f, 100.0f) <= CoinChancePercent ? DefaultCoin.Get() : DefaultGem.Get();
		ToActivate->ActivateCoin(true);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Viewport (currently unused - kept for CameraMain)
// ---------------------------------------------------------------------------------------------------------------------

void ABaseGameMode::FetchViewportSize()
{
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	if (!PlayerController)
	{
		return;
	}

	int32 SizeX = 0;
	int32 SizeY = 0;
	PlayerController->GetViewportSize(SizeX, SizeY);

	if (SizeX == 0 || SizeY == 0)
	{
		FTimerHandle RetryHandle;
		GetWorld()->GetTimerManager().SetTimer(RetryHandle, this, &ABaseGameMode::FetchViewportSize, 0.1f, false);
		return;
	}

	ViewportSize = FVector2D(SizeX, SizeY);
	OnViewportFetched.Broadcast();
}

// ---------------------------------------------------------------------------------------------------------------------
// Second chance: rewarded ad or gems. Every callback checks the phase, so late ad events can't touch a later run.
// ---------------------------------------------------------------------------------------------------------------------

void ABaseGameMode::LoadRewardedAd()
{
	if (CurrentGameState != EGameStateEnum::SecondChance)
	{
		return;
	}

	UAdsSubsystem* Ads = UAdsSubsystem::Get(this);
	if (!Ads || !Ads->IsRewardedAvailable())
	{
		bCanWatchAd = true;
		OnRewardedAdFailed.Broadcast(TEXT("Ads unavailable"));
		return;
	}

	FRewardedAdCallbacks Callbacks;

	Callbacks.OnShown = FSimpleDelegate::CreateWeakLambda(this, [this]()
	{
		if (CurrentGameState == EGameStateEnum::SecondChance)
		{
			bCanWatchAd = false; // one ad continue per run
			OnRewardedAdShown.Broadcast();
		}
	});

	Callbacks.OnEarned = FSimpleDelegate::CreateWeakLambda(this, [this]()
	{
		GrantSecondChance(); // no-op unless still in SecondChance
	});

	Callbacks.OnDismissedWithoutReward = FSimpleDelegate::CreateWeakLambda(this, [this]()
	{
		// Closed early without the reward: the run is over.
		if (CurrentGameState == EGameStateEnum::SecondChance)
		{
			EndGame();
		}
	});

	Callbacks.OnFailed = FOnRewardedAdError::CreateWeakLambda(this, [this](const FString& Message)
	{
		if (CurrentGameState == EGameStateEnum::SecondChance)
		{
			bCanWatchAd = true; // a failed ad isn't the player's fault - let them try again
			OnRewardedAdFailed.Broadcast(Message);
		}
	});

	Ads->ShowRewarded(Callbacks);
}

void ABaseGameMode::CancelRewardedAd()
{
	if (UAdsSubsystem* Ads = UAdsSubsystem::Get(this))
	{
		Ads->CancelRewarded();
	}
}

bool ABaseGameMode::TryContinueWithGems()
{
	if (CurrentGameState != EGameStateEnum::SecondChance)
	{
		return false;
	}

	UEconomySubsystem* Economy = UEconomySubsystem::Get(this);
	if (!Economy || !Economy->TrySpend(0, GemsNeededForSecondChance))
	{
		return false;
	}

	// Paying with gems: drop any rewarded ad still loading so it can't pop up mid-run.
	CancelRewardedAd();

	UE_LOG(LogFlappyHoop, Verbose, TEXT("Second chance bought for %d gem(s)"), GemsNeededForSecondChance);

	GemsSpent += GemsNeededForSecondChance;
	RetryCount++;
	GemsNeededForSecondChance = FMath::CeilToInt(BaseGemCost * FMath::Pow(GrowthRate, static_cast<float>(RetryCount)));

	GrantSecondChance();
	return true;
}

void ABaseGameMode::GrantSecondChance()
{
	if (CurrentGameState != EGameStateEnum::SecondChance)
	{
		return;
	}

	bTimeEnded = false;
	GetWorldTimerManager().ClearTimer(TimeUpFallbackTimer);
	RefillRoundTime();

	SetRunPhase(EGameStateEnum::Playing);
	OnSecondChanceGranted.Broadcast();
}

#if WITH_EDITOR
void ABaseGameMode::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(ABaseGameMode, CoinChancePercent))
	{
		CoinChancePercent = FMath::Clamp(CoinChancePercent, 0.0f, 100.0f);
		GemChancePercent = 100.0f - CoinChancePercent;
	}
}
#endif
