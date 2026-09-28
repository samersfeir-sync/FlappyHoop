// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "Engine/TimerHandle.h"
#include "Core/GameStateEnum.h"
#include "Data/BallType.h"
#include "BaseGameMode.generated.h"

class UGameWidget;
class UStaticMesh;
class ABall;
class ACoins;
class AGems;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnRunPhaseChanged, EGameStateEnum /*OldPhase*/, EGameStateEnum /*NewPhase*/);
DECLARE_MULTICAST_DELEGATE(FOnGameStarted);
DECLARE_MULTICAST_DELEGATE(FOnViewportFetched);
DECLARE_MULTICAST_DELEGATE(FOnGameReset);
DECLARE_MULTICAST_DELEGATE(FOnPointScored);
DECLARE_MULTICAST_DELEGATE(FOnTimeEnded);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnRoundTimeUpdated, float /*Fraction 1..0*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCoinCollected, bool /*bCoin*/);
DECLARE_MULTICAST_DELEGATE(FOnSecondChanceGranted);
DECLARE_MULTICAST_DELEGATE(FOnRewardedAdShown);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnRewardedAdFailed, const FString& /*Message*/);

/**
 * Owns the rules of a run: the run phase, the round timer, score/combo, collectibles picked up this run,
 * the second chance and the ads around it. Actors only report events (NotifyBasketPending, ScoreBasket,
 * NotifyBallLanded) and widgets react to OnRunPhaseChanged and the other delegates.
 */
UCLASS()
class FLAPPYHOOP_API ABaseGameMode : public AGameMode
{
	GENERATED_BODY()

public:

	/** The current game mode, or null (e.g. in the widget designer or a level without this mode). */
	static ABaseGameMode* Get(const UObject* WorldContextObject);

	// --- Events (bind in BeginPlay/NativeConstruct, unbind with RemoveAll in EndPlay/NativeDestruct) ---
	FOnRunPhaseChanged OnRunPhaseChanged;
	FOnGameStarted OnGameStarted;
	FOnGameReset OnGameReset;
	FOnTimeEnded OnTimeEnded;
	/** Fires every timer step while the clock runs, and when it is refilled. */
	FOnRoundTimeUpdated OnRoundTimeUpdated;
	FOnPointScored OnPointScored;
	/** Broadcast by coins/gems when picked up. */
	FOnCoinCollected OnCoinCollected;
	FOnSecondChanceGranted OnSecondChanceGranted;
	FOnRewardedAdShown OnRewardedAdShown;
	FOnRewardedAdFailed OnRewardedAdFailed;
	FOnViewportFetched OnViewportFetched;

	// --- Run flow ---
	EGameStateEnum GetCurrentGameState() const { return CurrentGameState; }

	/** MainMenu -> Playing. */
	void StartRun();

	/** Any phase -> MainMenu (Home button, Restart on the game-over panel). */
	void ReturnToMenu();

	/** Playing/SecondChance -> GameOver. Banks coins and the high score. Safe to call more than once. */
	void EndGame();

	// --- Round timer ---
	/** 1 = full time left, 0 = time is up. */
	float GetRoundTimeFraction() const;
	bool GetTimeEndedBool() const { return bTimeEnded; }

	// --- Events reported by actors ---
	/** A valid basket was made; input and the round timer freeze until ScoreBasket. */
	void NotifyBasketPending();
	bool IsBasketPending() const { return bBasketPending; }
	/** Awards the basket (after the hoop's celebration delay). */
	void ScoreBasket(bool bSwish);
	/** The ball hit the ground. Moves to SecondChance / GameOver when the time is up. */
	void NotifyBallLanded();
	/** True while the player is allowed to tap. */
	bool CanLaunchBall() const;

	// --- Score ---
	bool WasLastBasketSwish() const { return bLastBasketSwish; }
	int32 GetScoreMultiplier() const { return ScoreMultiplier; }
	void ResetScoreMultiplier() { ScoreMultiplier = 1; }
	int32 GetCurrentScore() const { return CurrentScore; }
	int32 GetHighScore() const { return HighScore; }

	// --- Collectibles picked up this run ---
	void AddCoin() { CollectedCoins++; }
	void AddGem();
	int32 GetCollectedCoins() const { return CollectedCoins; }
	int32 GetCollectedGems() const { return CollectedGems; }
	int32 GetGemsSpent() const { return GemsSpent; }

	// --- Second chance ---
	bool GetCanWatchAd() const { return bCanWatchAd; }
	int32 GetGemsNeededForSecondChance() const { return GemsNeededForSecondChance; }
	void LoadRewardedAd();
	/** Spends the gems and continues the run. Returns false if the player can't afford it. */
	bool TryContinueWithGems();

	// --- Misc ---
	FVector2D GetViewportSize() const { return ViewportSize; }

	/** The UI root (created in BeginPlay). */
	UGameWidget* GetGameWidget() const;

	/** OLD per-type ball meshes (read by the BallCatalog import button). */
	const TMap<EBallType, TObjectPtr<UStaticMesh>>& GetLegacyBallMeshes() const { return BallMeshes; }

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:

	// --- Phase ---
	EGameStateEnum CurrentGameState = EGameStateEnum::MainMenu;
	void SetRunPhase(EGameStateEnum NewPhase);

	/** Resets everything that belongs to a single run. */
	void ResetRunState();

	/** Continue the run after an ad reward or gem payment. */
	void GrantSecondChance();

	// --- Cached references ---
	UPROPERTY(Transient)
	TObjectPtr<UGameWidget> GameWidgetInstance = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UGameWidget> GameWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<ABall> DefaultBall = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<ACoins> DefaultCoin = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<AGems> DefaultGem = nullptr;

	void FetchViewportSize();
	FVector2D ViewportSize = FVector2D::ZeroVector;

	// --- Round timer / difficulty ---
	/** Each basket shortens the round by this fraction of the starting time. */
	UPROPERTY(EditDefaultsOnly, Category = "Parameters")
	float GameTimeReductionRate = 0.05f;

	/** Starting round time in seconds. */
	UPROPERTY(EditDefaultsOnly, Category = "Parameters")
	float MaxGameTime = 10.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Parameters", meta = (ClampMin = "0.5"))
	float MinGameTime = 3.0f;

	/** Current round length (shrinks as the score grows). */
	float CurrentRoundLength = 10.0f;
	float RoundTimeRemaining = 10.0f;
	bool bTimeEnded = false;
	bool bBasketPending = false;

	void ShortenRound();

	/**
	 * The round clock is a looping timer (not actor Tick): world timers always run for game modes,
	 * pause with the game, and catch up on slow frames.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Parameters", meta = (ClampMin = "0.005"))
	float RoundTimerStep = 0.02f;

	FTimerHandle RoundTimer;
	void StartRoundTimer();
	void StopRoundTimer();
	void RoundTimerStepElapsed();
	void RefillRoundTime();

	/**
	 * If the clock runs out while the ball is already lying still on the ground, no new landing is reported.
	 * After this many seconds with the time up (and no basket), the run proceeds as if the ball landed.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Parameters", meta = (ClampMin = "0.5"))
	float TimeUpLandingFallbackSeconds = 3.0f;

	FTimerHandle TimeUpFallbackTimer;
	void OnTimeUpFallback();

	// --- Scoring ---
	int32 CurrentScore = 0;
	int32 ScoreMultiplier = 1;
	int32 HighScore = 0;
	bool bLastBasketSwish = false;

	/** Points for a swish are multiplied by this (on top of the combo multiplier). */
	UPROPERTY(EditDefaultsOnly, Category = "Parameters", meta = (ClampMin = "1"))
	int32 SwishScoreMultiplier = 2;

	// --- Ball ---
	/**
	 * OLD: meshes per ball type. Only used as a fallback when a catalog entry has no mesh
	 * (e.g. before a BallCatalog asset is assigned). Can be cleared once the catalog is set up.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Ball")
	TMap<EBallType, TObjectPtr<UStaticMesh>> BallMeshes;

	/** Puts the selected ball's mesh on the ball actor. */
	void ApplyBallSettings();

	// --- Collectibles ---
	int32 CollectedCoins = 0;
	int32 CollectedGems = 0;
	int32 GemsSpent = 0;

	void MaybeSpawnCollectible();

	UPROPERTY(EditDefaultsOnly, Category = "Collectibles", meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0"))
	float CollectibleChancePercent = 25.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Collectibles", meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0"))
	float CoinChancePercent = 75.0f;

	UPROPERTY(VisibleDefaultsOnly, Category = "Collectibles")
	float GemChancePercent = 25.0f;

	// --- Ads (pacing settings; the ads themselves live in UAdsSubsystem) ---
	/** Show an interstitial when returning to the menu after this many finished games. */
	UPROPERTY(EditDefaultsOnly, Category = "Ads", meta = (ClampMin = "1"))
	int32 InterstitialEveryNGames = 3;

	/** Minimum real seconds between two interstitials. */
	UPROPERTY(EditDefaultsOnly, Category = "Ads", meta = (ClampMin = "0.0"))
	float InterstitialCooldownSeconds = 60.0f;

	/** Retry delay after an interstitial failed to load. */
	UPROPERTY(EditDefaultsOnly, Category = "Ads", meta = (ClampMin = "5.0"))
	float InterstitialRetrySeconds = 30.0f;

	// --- Second chance ---
	void CancelRewardedAd();

	/** Whether the "watch ad" continue is available at the start of each run (one per run). */
	UPROPERTY(EditDefaultsOnly, Category = "Second Chance")
	bool bCanWatchAd = true;

	bool bCanWatchAdDefault = true;

	UPROPERTY(EditDefaultsOnly, Category = "Second Chance", meta = (ClampMin = "1"))
	int32 BaseGemCost = 1;

	/** Gem cost multiplies by this after every gem continue in the same run. */
	UPROPERTY(EditDefaultsOnly, Category = "Second Chance", meta = (ClampMin = "1.0"))
	float GrowthRate = 1.5f;

	int32 GemsNeededForSecondChance = 1;
	int32 RetryCount = 0;
};
