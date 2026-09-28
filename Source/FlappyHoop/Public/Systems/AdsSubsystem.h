// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Ads/AGRewardItem.h"
#include "Interface/AGBannerAdInterface.h"
#include "Interface/AGInterstitialAdInterface.h"
#include "Interface/AGRewardedAdInterface.h"
#include "Consent/AGConsentLibrary.h"
#include "Engine/TimerHandle.h"
#include "AdsSubsystem.generated.h"

class FTimerManager;

DECLARE_DELEGATE_OneParam(FOnRewardedAdError, const FString& /*Message*/);

/** The consent step finished (form shown/dismissed, not needed, or failed). */
DECLARE_MULTICAST_DELEGATE(FOnAdConsentFinished);

/** Callbacks for one rewarded-ad attempt. Unbound ones are simply skipped. */
struct FRewardedAdCallbacks
{
	/** The ad loaded and is about to cover the screen. */
	FSimpleDelegate OnShown;
	/** The player earned the reward. */
	FSimpleDelegate OnEarned;
	/** The ad was closed without the reward. */
	FSimpleDelegate OnDismissedWithoutReward;
	/** The ad failed to load or to show. */
	FOnRewardedAdError OnFailed;
};

/**
 * All AdMob usage: the banner, interstitials and rewarded ads. Knows nothing about game rules;
 * the game mode tells it when a run finished and asks it for rewarded ads.
 * Respects "remove ads" (the banner and interstitials stop; rewarded ads stay, since the player opts in).
 *
 * Consent (Google UMP, required in the EEA/UK/Switzerland): StartConsentFlow() asks Google whether consent is
 * needed and shows its form if so. No ad is requested until UMP says ads may be requested; a banner/interstitial
 * asked for earlier is started as soon as it does.
 */
UCLASS()
class FLAPPYHOOP_API UAdsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	static UAdsSubsystem* Get(const UObject* WorldContextObject);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// --- Consent (UMP) ---
	/** Asks for consent info and shows Google's consent form when required. Safe to call more than once. */
	void StartConsentFlow();

	/** True once UMP allows ad requests (always true off-mobile, where ads are no-ops anyway). */
	bool CanRequestAds() const;

	/** Google requires a way to change the choice later when this is true (show a "Privacy options" button). */
	bool IsPrivacyOptionsRequired() const;

	/** Opens Google's privacy options form. Only call from a button press. */
	void ShowPrivacyOptions();

	FOnAdConsentFinished OnConsentFinished;

	/** True when the player bought "remove ads". */
	bool AreAdsRemoved() const;

	/** Called by the store after "remove ads" was bought or restored. */
	void HandleAdsRemoved();

	// --- Banner ---
	void ShowBanner();
	void DestroyBanner();

	// --- Interstitials ---
	/** Pacing, set by the game mode from its Blueprint settings. */
	void ConfigureInterstitials(int32 EveryNGames, float CooldownSeconds, float RetrySeconds);
	void PreloadInterstitial();
	/** Counts a finished run towards the next interstitial. */
	void NotifyRunFinished() { GamesSinceLastInterstitial++; }
	/** Shows an interstitial if enough runs finished, the cooldown passed and one is loaded. */
	void TryShowInterstitial();

	// --- Rewarded ---
	bool IsRewardedAvailable() const { return !RewardedADUnitID.IsEmpty(); }
	/** Loads a rewarded ad and shows it as soon as it's ready. Replaces any attempt still in flight. */
	void ShowRewarded(const FRewardedAdCallbacks& Callbacks);
	/** Forgets the current attempt; its late callbacks are ignored. */
	void CancelRewarded();

private:

	void InitializeADUnits();

	// --- Consent ---
	bool bConsentFlowStarted = false;
	bool bConsentFlowDone = false;

	/** Wanted before consent allowed it; started in FinishConsent. */
	bool bBannerWanted = false;
	bool bInterstitialWanted = false;

	UAGConsentLibrary::FAGConsentDelegate ConsentInfoUpdatedDelegate;
	UAGConsentLibrary::FAGConsentErrorDelegate ConsentInfoErrorDelegate;
	UAGConsentLibrary::FAGConsentDelegate ConsentFormDismissedDelegate;
	UAGConsentLibrary::FAGConsentErrorDelegate ConsentFormErrorDelegate;
	UAGConsentLibrary::FAGConsentDelegate PrivacyFormClosedDelegate;
	UAGConsentLibrary::FAGConsentErrorDelegate PrivacyFormErrorDelegate;

	UFUNCTION()
	void OnConsentInfoUpdated();

	UFUNCTION()
	void OnConsentInfoError(int ErrorCode, FString ErrorMessage);

	UFUNCTION()
	void OnConsentFormDismissed();

	UFUNCTION()
	void OnConsentFormError(int ErrorCode, FString ErrorMessage);

	UFUNCTION()
	void OnPrivacyFormClosed();

	UFUNCTION()
	void OnPrivacyFormError(int ErrorCode, FString ErrorMessage);

	void FinishConsent();

	FString BannerADUnitID;
	FString InterstitialADUnitID;
	FString RewardedADUnitID;

	FTimerManager* GetTimers() const;

	// --- Banner ---
	UPROPERTY(Transient)
	TScriptInterface<IAGBannerAdInterface> BannerAdInterface;

	// --- Interstitial ---
	UPROPERTY(Transient)
	TScriptInterface<IAGInterstitialAdInterface> InterstitialAdInterface;

	int32 InterstitialEveryNGames = 3;
	float InterstitialCooldownSeconds = 60.0f;
	float InterstitialRetrySeconds = 30.0f;

	int32 GamesSinceLastInterstitial = 0;
	double LastInterstitialShownTime = -1.0e9;
	bool bInterstitialReady = false;
	bool bInterstitialLoading = false;
	FTimerHandle InterstitialRetryTimer;

	bool InterstitialsDisabled() const;

	UFUNCTION()
	void OnInterstitialLoaded();

	UFUNCTION()
	void OnInterstitialFailedToLoad(int ErrorCode, FString ErrorMessage);

	UFUNCTION()
	void OnInterstitialClosed();

	UFUNCTION()
	void OnInterstitialFailedToShow(int ErrorCode, FString ErrorMessage);

	// --- Rewarded ---
	UPROPERTY(Transient)
	TScriptInterface<IAGRewardedAdInterface> RewardedAdInterface;

	FRewardedAdCallbacks RewardedCallbacks;
	bool bRewardEarned = false;
	FTimerHandle RewardedClosedTimer;

	UFUNCTION()
	void OnRewardedLoaded();

	UFUNCTION()
	void OnRewardedEarned(FRewardItem Reward);

	UFUNCTION()
	void OnRewardedClosed();

	UFUNCTION()
	void OnRewardedFailed(int ErrorCode, FString ErrorMessage);

	void ResolveRewardedClosed();
	void FailRewarded(const FString& Message);
};
