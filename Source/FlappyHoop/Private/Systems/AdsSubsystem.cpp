// Fill out your copyright notice in the Description page of Project Settings.

#include "Systems/AdsSubsystem.h"
#include "Core/FlappyHoop.h"
#include "Systems/SaveSubsystem.h"
#include "Framework/GameInfoInstance.h"
#include "Ads/AGAdLibrary.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "TimerManager.h"

UAdsSubsystem* UAdsSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UAdsSubsystem>() : nullptr;
}

void UAdsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Collection.InitializeDependency<USaveSubsystem>();
	Super::Initialize(Collection);

	InitializeADUnits();
}

void UAdsSubsystem::Deinitialize()
{
	if (FTimerManager* Timers = GetTimers())
	{
		Timers->ClearAllTimersForObject(this);
	}

	Super::Deinitialize();
}

FTimerManager* UAdsSubsystem::GetTimers() const
{
	// Game-instance timers survive map loads, unlike world timers.
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? &GameInstance->GetTimerManager() : nullptr;
}

void UAdsSubsystem::InitializeADUnits()
{
#if PLATFORM_ANDROID
	BannerADUnitID = TEXT("ca-app-pub-5279001044325653/5546944296");
	InterstitialADUnitID = TEXT("ca-app-pub-5279001044325653/4163109237");
	RewardedADUnitID = TEXT("ca-app-pub-5279001044325653/7632947939");
#elif PLATFORM_IOS
	BannerADUnitID = TEXT("");
	InterstitialADUnitID = TEXT("");
	RewardedADUnitID = TEXT("");
#endif
}

// ---------------------------------------------------------------------------------------------------------------------
// Consent (Google UMP)
// ---------------------------------------------------------------------------------------------------------------------

void UAdsSubsystem::StartConsentFlow()
{
	if (bConsentFlowStarted)
	{
		return;
	}
	bConsentFlowStarted = true;

#if PLATFORM_ANDROID || PLATFORM_IOS
	ConsentInfoUpdatedDelegate.BindDynamic(this, &UAdsSubsystem::OnConsentInfoUpdated);
	ConsentInfoErrorDelegate.BindDynamic(this, &UAdsSubsystem::OnConsentInfoError);

	const UGameInfoInstance* GameInstance = Cast<UGameInfoInstance>(GetGameInstance());
	const bool bDebugEEA = !UE_BUILD_SHIPPING && GameInstance && GameInstance->ShouldDebugConsentAsEEA();

	if (bDebugEEA)
	{
		// Needs your device in the plugin's "Test Device IDs" (Project Settings > Admob Goodies).
		UE_LOG(LogFlappyHoop, Log, TEXT("[Consent] Requesting consent info (debug: pretending to be in the EEA)"));
		UAGConsentLibrary::RequestConsentInfoDebugGeography(EDebugGeography::EEA, ConsentInfoUpdatedDelegate, ConsentInfoErrorDelegate);
	}
	else
	{
		UE_LOG(LogFlappyHoop, Log, TEXT("[Consent] Requesting consent info"));
		UAGConsentLibrary::RequestConsentInfo(ConsentInfoUpdatedDelegate, ConsentInfoErrorDelegate, false);
	}
#else
	FinishConsent();
#endif
}

void UAdsSubsystem::OnConsentInfoUpdated()
{
	const EConsentStatus Status = UAGConsentLibrary::GetConsentStatus();
	UE_LOG(LogFlappyHoop, Log, TEXT("[Consent] Info updated, status %d"), static_cast<int32>(Status));

	if (Status == EConsentStatus::Required)
	{
		ConsentFormDismissedDelegate.BindDynamic(this, &UAdsSubsystem::OnConsentFormDismissed);
		ConsentFormErrorDelegate.BindDynamic(this, &UAdsSubsystem::OnConsentFormError);
		UAGConsentLibrary::LoadAndShowConsentForm(ConsentFormDismissedDelegate, ConsentFormErrorDelegate);
		return;
	}

	FinishConsent();
}

void UAdsSubsystem::OnConsentInfoError(int ErrorCode, FString ErrorMessage)
{
	// E.g. offline. A choice from an earlier session may still allow ads (CanRequestAds decides).
	UE_LOG(LogFlappyHoop, Warning, TEXT("[Consent] Info error %d: %s"), ErrorCode, *ErrorMessage);
	FinishConsent();
}

void UAdsSubsystem::OnConsentFormDismissed()
{
	UE_LOG(LogFlappyHoop, Log, TEXT("[Consent] Form dismissed"));
	FinishConsent();
}

void UAdsSubsystem::OnConsentFormError(int ErrorCode, FString ErrorMessage)
{
	UE_LOG(LogFlappyHoop, Warning, TEXT("[Consent] Form error %d: %s"), ErrorCode, *ErrorMessage);
	FinishConsent();
}

void UAdsSubsystem::FinishConsent()
{
	bConsentFlowDone = true;

	const bool bAllowed = CanRequestAds();
	UE_LOG(LogFlappyHoop, Log, TEXT("[Consent] Done. Ads allowed: %s, privacy options required: %s"),
		bAllowed ? TEXT("yes") : TEXT("no"), IsPrivacyOptionsRequired() ? TEXT("yes") : TEXT("no"));

	if (bAllowed)
	{
		if (bBannerWanted)
		{
			ShowBanner();
		}
		if (bInterstitialWanted)
		{
			PreloadInterstitial();
		}
	}

	OnConsentFinished.Broadcast();
}

bool UAdsSubsystem::CanRequestAds() const
{
#if PLATFORM_ANDROID || PLATFORM_IOS
	return UAGConsentLibrary::CanRequestAds();
#else
	return true;
#endif
}

bool UAdsSubsystem::IsPrivacyOptionsRequired() const
{
#if PLATFORM_ANDROID || PLATFORM_IOS
	return UAGConsentLibrary::IsPrivacyOptionsRequired();
#else
	return false;
#endif
}

void UAdsSubsystem::ShowPrivacyOptions()
{
#if PLATFORM_ANDROID || PLATFORM_IOS
	PrivacyFormClosedDelegate.BindDynamic(this, &UAdsSubsystem::OnPrivacyFormClosed);
	PrivacyFormErrorDelegate.BindDynamic(this, &UAdsSubsystem::OnPrivacyFormError);
	UAGConsentLibrary::ShowPrivacyOptionsForm(PrivacyFormClosedDelegate, PrivacyFormErrorDelegate);
#endif
}

void UAdsSubsystem::OnPrivacyFormClosed()
{
	// The choice may have changed: start ads now if they became allowed.
	FinishConsent();
}

void UAdsSubsystem::OnPrivacyFormError(int ErrorCode, FString ErrorMessage)
{
	UE_LOG(LogFlappyHoop, Warning, TEXT("[Consent] Privacy options error %d: %s"), ErrorCode, *ErrorMessage);
}

bool UAdsSubsystem::AreAdsRemoved() const
{
	const USaveSubsystem* Save = GetGameInstance() ? GetGameInstance()->GetSubsystem<USaveSubsystem>() : nullptr;
	return Save && Save->GetProgression().bNoAds;
}

void UAdsSubsystem::HandleAdsRemoved()
{
	DestroyBanner();

	if (FTimerManager* Timers = GetTimers())
	{
		Timers->ClearTimer(InterstitialRetryTimer);
	}
	bInterstitialReady = false;
	bInterstitialLoading = false;
	InterstitialAdInterface = nullptr;
}

// ---------------------------------------------------------------------------------------------------------------------
// Banner
// ---------------------------------------------------------------------------------------------------------------------

void UAdsSubsystem::ShowBanner()
{
	if (AreAdsRemoved() || BannerADUnitID.IsEmpty() || BannerAdInterface)
	{
		return;
	}

	if (!CanRequestAds())
	{
		bBannerWanted = true; // started once consent allows it
		return;
	}

	BannerAdInterface = UAGAdLibrary::MakeBannerAd(BannerADUnitID, EAdSizeType::Banner, EAdPosition::Bottom);
	if (BannerAdInterface)
	{
		BannerAdInterface->LoadAd(true);
	}
}

void UAdsSubsystem::DestroyBanner()
{
	if (BannerAdInterface)
	{
		BannerAdInterface->Destroy();
		BannerAdInterface = nullptr;
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Interstitials: preloaded once, shown on return to the menu every N finished runs, with a real-time cooldown.
// ---------------------------------------------------------------------------------------------------------------------

void UAdsSubsystem::ConfigureInterstitials(int32 EveryNGames, float CooldownSeconds, float RetrySeconds)
{
	InterstitialEveryNGames = FMath::Max(1, EveryNGames);
	InterstitialCooldownSeconds = FMath::Max(0.0f, CooldownSeconds);
	InterstitialRetrySeconds = FMath::Max(5.0f, RetrySeconds);
}

bool UAdsSubsystem::InterstitialsDisabled() const
{
	return AreAdsRemoved() || InterstitialADUnitID.IsEmpty();
}

void UAdsSubsystem::PreloadInterstitial()
{
	if (InterstitialsDisabled() || bInterstitialReady || bInterstitialLoading)
	{
		return;
	}

	if (!CanRequestAds())
	{
		bInterstitialWanted = true; // loaded once consent allows it
		return;
	}

	if (!InterstitialAdInterface)
	{
		InterstitialAdInterface = UAGAdLibrary::MakeInterstitialAd(InterstitialADUnitID);
		if (!InterstitialAdInterface)
		{
			return;
		}

		FOnInterstitialAdLoadedDelegate Loaded;
		Loaded.BindDynamic(this, &UAdsSubsystem::OnInterstitialLoaded);
		InterstitialAdInterface->BindEventToOnAdLoaded(Loaded);

		FOnInterstitialAdFailedToLoadDelegate FailedToLoad;
		FailedToLoad.BindDynamic(this, &UAdsSubsystem::OnInterstitialFailedToLoad);
		InterstitialAdInterface->BindEventToOnAdFailedToLoad(FailedToLoad);

		FOnInterstitialAdClosedDelegate Closed;
		Closed.BindDynamic(this, &UAdsSubsystem::OnInterstitialClosed);
		InterstitialAdInterface->BindEventToOnAdClosed(Closed);

		FOnInterstitialAdFailedToShowDelegate FailedToShow;
		FailedToShow.BindDynamic(this, &UAdsSubsystem::OnInterstitialFailedToShow);
		InterstitialAdInterface->BindEventToOnAdFailedToShow(FailedToShow);
	}

	bInterstitialLoading = true;
	InterstitialAdInterface->LoadAd();
}

void UAdsSubsystem::TryShowInterstitial()
{
	if (InterstitialsDisabled())
	{
		return;
	}

	const bool bEnoughGames = GamesSinceLastInterstitial >= InterstitialEveryNGames;
	const bool bCooledDown = (FPlatformTime::Seconds() - LastInterstitialShownTime) >= InterstitialCooldownSeconds;
	if (!bEnoughGames || !bCooledDown)
	{
		return;
	}

	if (!bInterstitialReady || !InterstitialAdInterface || !InterstitialAdInterface->IsLoaded())
	{
		PreloadInterstitial();
		return;
	}

	bInterstitialReady = false;
	GamesSinceLastInterstitial = 0;
	LastInterstitialShownTime = FPlatformTime::Seconds();
	InterstitialAdInterface->Show();
}

void UAdsSubsystem::OnInterstitialLoaded()
{
	bInterstitialLoading = false;
	bInterstitialReady = true;
}

void UAdsSubsystem::OnInterstitialFailedToLoad(int ErrorCode, FString ErrorMessage)
{
	UE_LOG(LogFlappyHoop, Warning, TEXT("[Ads] Interstitial failed to load (%d): %s"), ErrorCode, *ErrorMessage);

	bInterstitialLoading = false;
	bInterstitialReady = false;

	if (FTimerManager* Timers = GetTimers())
	{
		Timers->SetTimer(InterstitialRetryTimer, FTimerDelegate::CreateUObject(this, &UAdsSubsystem::PreloadInterstitial),
			InterstitialRetrySeconds, false);
	}
}

void UAdsSubsystem::OnInterstitialClosed()
{
	// A fresh ad object for the next load rather than reusing the one that was shown.
	InterstitialAdInterface = nullptr;
	PreloadInterstitial();
}

void UAdsSubsystem::OnInterstitialFailedToShow(int ErrorCode, FString ErrorMessage)
{
	UE_LOG(LogFlappyHoop, Warning, TEXT("[Ads] Interstitial failed to show (%d): %s"), ErrorCode, *ErrorMessage);
	InterstitialAdInterface = nullptr;
	PreloadInterstitial();
}

// ---------------------------------------------------------------------------------------------------------------------
// Rewarded
// ---------------------------------------------------------------------------------------------------------------------

void UAdsSubsystem::ShowRewarded(const FRewardedAdCallbacks& Callbacks)
{
	CancelRewarded();
	RewardedCallbacks = Callbacks;

	if (RewardedADUnitID.IsEmpty())
	{
		FailRewarded(TEXT("Ads unavailable"));
		return;
	}

	if (!CanRequestAds())
	{
		FailRewarded(TEXT("Ads aren't available right now."));
		return;
	}

	RewardedAdInterface = UAGAdLibrary::MakeRewardedAd(RewardedADUnitID);
	if (!RewardedAdInterface)
	{
		FailRewarded(TEXT("Ads unavailable"));
		return;
	}

	// Bind everything before LoadAd so no callback can be missed.
	FOnRewardedAdLoadedDelegate Loaded;
	Loaded.BindDynamic(this, &UAdsSubsystem::OnRewardedLoaded);
	RewardedAdInterface->BindEventToOnAdLoaded(Loaded);

	FOnRewardedAdUserEarhedRewardDelegate Earned;
	Earned.BindDynamic(this, &UAdsSubsystem::OnRewardedEarned);
	RewardedAdInterface->BindEventToOnUserEarnedReward(Earned);

	FOnRewardedAdClosedDelegate Closed;
	Closed.BindDynamic(this, &UAdsSubsystem::OnRewardedClosed);
	RewardedAdInterface->BindEventToOnAdClosed(Closed);

	FOnRewardedAdFailedToLoadDelegate FailedToLoad;
	FailedToLoad.BindDynamic(this, &UAdsSubsystem::OnRewardedFailed);
	RewardedAdInterface->BindEventToOnAdFailedToLoad(FailedToLoad);

	FOnRewardedAdFailedToShowDelegate FailedToShow;
	FailedToShow.BindDynamic(this, &UAdsSubsystem::OnRewardedFailed);
	RewardedAdInterface->BindEventToOnAdFailedToShow(FailedToShow);

	RewardedAdInterface->LoadAd();
}

void UAdsSubsystem::CancelRewarded()
{
	if (FTimerManager* Timers = GetTimers())
	{
		Timers->ClearTimer(RewardedClosedTimer);
	}

	RewardedAdInterface = nullptr;
	RewardedCallbacks = FRewardedAdCallbacks();
	bRewardEarned = false;
}

void UAdsSubsystem::OnRewardedLoaded()
{
	if (!RewardedAdInterface)
	{
		return; // cancelled
	}

	const FSimpleDelegate OnShown = RewardedCallbacks.OnShown; // copy: the callback may cancel
	OnShown.ExecuteIfBound();

	if (RewardedAdInterface) // OnShown may have cancelled
	{
		RewardedAdInterface->Show();
	}
}

void UAdsSubsystem::OnRewardedEarned(FRewardItem Reward)
{
	if (!RewardedAdInterface || bRewardEarned)
	{
		return;
	}

	bRewardEarned = true;
	const FSimpleDelegate OnEarned = RewardedCallbacks.OnEarned; // copy: the callback may cancel
	OnEarned.ExecuteIfBound();
}

void UAdsSubsystem::OnRewardedClosed()
{
	if (!RewardedAdInterface)
	{
		return;
	}

	// The reward callback can arrive just after the close callback on some devices; give it a moment.
	if (FTimerManager* Timers = GetTimers())
	{
		Timers->SetTimer(RewardedClosedTimer, FTimerDelegate::CreateUObject(this, &UAdsSubsystem::ResolveRewardedClosed), 0.5f, false);
	}
	else
	{
		ResolveRewardedClosed();
	}
}

void UAdsSubsystem::ResolveRewardedClosed()
{
	const bool bEarned = bRewardEarned;
	const FSimpleDelegate OnDismissed = RewardedCallbacks.OnDismissedWithoutReward;

	CancelRewarded();

	if (!bEarned)
	{
		OnDismissed.ExecuteIfBound();
	}
}

void UAdsSubsystem::OnRewardedFailed(int ErrorCode, FString ErrorMessage)
{
	if (!RewardedAdInterface)
	{
		return;
	}

	UE_LOG(LogFlappyHoop, Warning, TEXT("[Ads] Rewarded ad failed (%d): %s"), ErrorCode, *ErrorMessage);
	FailRewarded(ErrorMessage);
}

void UAdsSubsystem::FailRewarded(const FString& Message)
{
	const FOnRewardedAdError OnFailed = RewardedCallbacks.OnFailed;
	CancelRewarded();
	OnFailed.ExecuteIfBound(Message);
}
