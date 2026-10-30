// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GameInfoInstance.generated.h"

class UMySaveGame;
class USoundBase;
class UBallCatalog;

/**
 * Thin game instance: holds the Blueprint-edited settings the subsystems read, and runs the startup sequence
 * (music + banner once the startup movie is done). The actual work lives in the subsystems:
 * USaveSubsystem, UEconomySubsystem, UStoreSubsystem, UAdsSubsystem, UFlappyAudioSubsystem.
 */
UCLASS()
class FLAPPYHOOP_API UGameInfoInstance : public UGameInstance
{
	GENERATED_BODY()

public:

	virtual void Init() override;
	virtual void OnStart() override;
	virtual void Shutdown() override;

	TSubclassOf<UMySaveGame> GetSaveGameClass() const { return SaveGameClass; }
	USoundBase* GetGameMusic() const { return GameMusic; }
	int32 GetMaxBillingReconnectAttempts() const { return MaxBillingReconnectAttempts; }
	const UBallCatalog* GetBallCatalog() const { return BallCatalog; }
	bool ShouldDebugConsentAsEEA() const { return bDebugConsentAsEEA; }
	bool AreAdsChildDirected() const { return bChildDirectedAds; }
	const FString& GetGameCenterLeaderboardID() const { return GameCenterLeaderboardID; }
	const FString& GetGameCenterHardLeaderboardID() const { return GameCenterHardLeaderboardID; }
	bool ShouldSignInToGameServicesAtStartup() const { return bSignInToGameServicesAtStartup; }

private:

	/** Save object class; its defaults are the starting progression (including the shop's ball list). */
	UPROPERTY(EditDefaultsOnly, Category = "Save")
	TSubclassOf<UMySaveGame> SaveGameClass;

	/** Every ball in the game (mesh, shop icon, price). */
	UPROPERTY(EditDefaultsOnly, Category = "Shop")
	TObjectPtr<UBallCatalog> BallCatalog;

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	TObjectPtr<USoundBase> GameMusic;

	UPROPERTY(EditDefaultsOnly, Category = "Billing")
	int32 MaxBillingReconnectAttempts = 6;

	/**
	 * Testing only (ignored in Shipping): pretend the device is in the EEA so Google's consent form shows.
	 * Your device must be listed in Project Settings > Admob Goodies > Test Device IDs.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Ads")
	bool bDebugConsentAsEEA = false;

	/**
	 * Google Play Families: on when the app's target audience includes children. Every player then gets
	 * child-directed, non-personalized, G-rated ads (no consent form is needed). Turn off only if the
	 * target audience in Play Console is 13+.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Ads")
	bool bChildDirectedAds = true;

	/**
	 * Game Center leaderboard ID for the high score (App Store Connect > Game Center > Leaderboards).
	 * Android uses the "HighScore" entry of the Leaderboard Map in Project Settings > Android > Google Play Services.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Leaderboards")
	FString GameCenterLeaderboardID = TEXT("flappyhoop.highscore");

	/**
	 * Game Center leaderboard ID for Hard mode. The Easy board is GameCenterLeaderboardID (old scores stay there).
	 * Android uses the "HighScoreHard" entry of the Leaderboard Map.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Leaderboards")
	FString GameCenterHardLeaderboardID = TEXT("flappyhoop.highscore.hard");

	/**
	 * Sign in to Google Play Games / Game Center when the game starts (returning players are signed in silently;
	 * new players see the sign-in prompt once). Off: sign-in only happens when the leaderboard button is pressed.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Leaderboards")
	bool bSignInToGameServicesAtStartup = true;

	/**
	 * Render resolution on phones (r.MobileContentScaleFactor), set from code so device profiles cannot override it.
	 * 0 = the phone's native resolution (sharpest). Higher numbers are lower resolutions, e.g. 1.5 or 2 for weak phones.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Display", meta = (ClampMin = "0"))
	float MobileResolutionScale = 0.0f;

	void ApplyMobileResolution();

	void HandleStartupMovieFinished();
	void StartMusicAndBanner();
	bool bStartupHandled = false;
};
