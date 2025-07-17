// Fill out your copyright notice in the Description page of Project Settings.


#include "GameInfoInstance.h"
#include "Kismet/GameplayStatics.h"
#include "MySaveGame.h"
#include "MoviePlayer.h"
#include "Ads/AGAdLibrary.h"
#include "Components/AudioComponent.h"
#include "FunctionsLibrary.h"
#include "MGAndroidPurchaseHistoryRecord.h"
#include "MGAndroidPurchase.h"

void UGameInfoInstance::OnPurchaseUpdated(UMGAndroidBillingResult* Result, const TArray<UMGAndroidPurchase*>& Purchases)
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("OnPurchaseUpdated functions called.")));

	if (UFunctionsLibrary::BillingResponseOK(Result))
	{
		for (UMGAndroidPurchase* Purchase : Purchases)
		{
			if (!Purchase->IsAcknowledged() && Purchase->GetPurchaseState() == EMGAndroidPurchaseState::Purchased)
			{
				if (IsBillingClientReady())
				{
					CurrentPurchase = Purchase;
					OnAcknowledgeCompletedDelegate.BindDynamic(this, &UGameInfoInstance::OnAcknowledgeCompleted);
					AndroidBillingClient->AcknowledgePurchase(CurrentPurchase->GetPurchaseToken(), OnAcknowledgeCompletedDelegate);
					GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Acknowledging Purchase: %s"), *PurchaseToken));
				}
			}
		}
	}
}

void UGameInfoInstance::OnAcknowledgeCompleted(UMGAndroidBillingResult* Result)
{
	if (UFunctionsLibrary::BillingResponseOK(Result))
	{
		if (IsBillingClientReady() && CurrentPurchase)
		{
			EMGAndroidPurchaseState PurchaseState = CurrentPurchase->GetPurchaseState();
			switch (PurchaseState)
			{
			case EMGAndroidPurchaseState::Unspecified:
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Purchase State: Unspecified"));
				break;

			case EMGAndroidPurchaseState::Purchased:
			{				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Purchase State: Purchased"));

				PurchaseToken = CurrentPurchase->GetPurchaseToken();

				FString Message = FString::Printf(TEXT("OnPurchaseSuccessfulDelegate IsBound? %s"), 
					OnPurchaseSuccessfulDelegate.IsBound() ? TEXT("true") : TEXT("false"));
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, Message);

				OnPurchaseSuccessfulDelegate.ExecuteIfBound();

				break;
			}

			case EMGAndroidPurchaseState::Pending:
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("Purchase State: Pending"));
				break;
			default:
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::White, TEXT("Purchase State: Unknown"));
				break;
			}
		}
	}
}

void UGameInfoInstance::SaveUserProgression(FUserProgression& NewUserProgression)
{
	if (SaveGame)
	{
		UserProgression = NewUserProgression;
		SaveGame->UserProgression = NewUserProgression;
		UGameplayStatics::SaveGameToSlot(SaveGame, "UserProgressionSlot", 0);
	}
}

void UGameInfoInstance::LoadUserProgression()
{
	if (UGameplayStatics::DoesSaveGameExist(TEXT("UserProgressionSlot"), 0))
		SaveGame = Cast<UMySaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("UserProgressionSlot"), 0));

	else
		SaveGame = Cast<UMySaveGame>(
			UGameplayStatics::CreateSaveGameObject(SaveGameClass));

	if (SaveGame)
	{
		UserProgression = SaveGame->UserProgression;
	}
}

FUserProgression& UGameInfoInstance::GetUserProgression()
{
	return UserProgression;
}

bool UGameInfoInstance::IsBillingClientReady() const
{
#if PLATFORM_ANDROID
	const bool bValid = AndroidBillingClient != nullptr;
	const bool bNativeValid = bValid && AndroidBillingClient->IsNativeObjectValid();
	const bool bReady = bNativeValid && AndroidBillingClient->IsReady();

	FString Message = FString::Printf(TEXT("IsBillingClientReady: %s"), bReady ? TEXT("true") : TEXT("false"));
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, Message);

	return bReady;
#else
	return false;
#endif
}

void UGameInfoInstance::Init()
{
	Super::Init();

	LoadUserProgression();
	InitializeADUnits();
	GetMoviePlayer()->OnMoviePlaybackFinished().AddUObject(this, &UGameInfoInstance::OnMoviePlaybackFinished);

	OnPurchaseUpdatedDelegate.BindDynamic(this, &UGameInfoInstance::OnPurchaseUpdated);
	AndroidBillingClient = UMGAndroidBillingLibrary::CreateAndroidBillingClient(OnPurchaseUpdatedDelegate);
	StartConnection();

	MusicAudioComponent = NewObject<UAudioComponent>(this);

	if (MusicAudioComponent)
	{
		MusicAudioComponent->SetSound(GameMusic);
		MusicAudioComponent->bAutoActivate = false;
		MusicAudioComponent->bIsUISound = true;
		MusicAudioComponent->RegisterComponent();
	}
}

void UGameInfoInstance::OnMoviePlaybackFinished()
{
	bool bMusicMuted = UserProgression.bIsMusicMuted;

	if (!bMusicMuted)
		PlayBackgroundMusic();

	if (UserProgression.bNoAds)
		return;

	//banner ad
	BannerAdInterface = UAGAdLibrary::MakeBannerAd(
		BannerADUnitID,
		EAdSizeType::Banner,
		EAdPosition::Bottom
	);

	if (BannerAdInterface)
	{
		BannerAdInterface->LoadAd(true);
	}
}

void UGameInfoInstance::PlayBackgroundMusic()
{
	if (MusicAudioComponent)
	{
		MusicAudioComponent->Play();
	}
}

void UGameInfoInstance::StopBackgroundMusic()
{
	if (MusicAudioComponent)
	{
		MusicAudioComponent->Stop();
	}
}

void UGameInfoInstance::InitializeADUnits()
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

void UGameInfoInstance::RetryConnection()
{
	if (IsBillingClientReady())
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Connection failed, retrying")));
		StartConnection();
	}
}

void UGameInfoInstance::StartConnection()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Trying to Connect...")));
	OnBillingSetupFinishedDelegate.BindDynamic(this, &UGameInfoInstance::BillingSetupFinished);
	OnBillingDisconnectedDelegate.BindDynamic(this, &UGameInfoInstance::RetryConnection);
	AndroidBillingClient->StartConnection(OnBillingSetupFinishedDelegate, OnBillingDisconnectedDelegate);
}

void UGameInfoInstance::BillingSetupFinished(UMGAndroidBillingResult* Result)
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Billing Setup Finished")));

	if (UFunctionsLibrary::BillingResponseOK(Result))
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Querying Purchase History")));
		OnPurchaseHistoryFetchedDelegate.BindDynamic(this, &UGameInfoInstance::RestorePurchases);
		AndroidBillingClient->QueryPurchaseHistory(EMGSkuType::Purchase, OnPurchaseHistoryFetchedDelegate);
	}
}

void UGameInfoInstance::RestorePurchases(UMGAndroidBillingResult* Result, 
	const TArray<UMGAndroidPurchaseHistoryRecord*>& Records)
{
	if (UFunctionsLibrary::BillingResponseOK(Result))
	{
		if (Records.IsEmpty())
			return;

		for (UMGAndroidPurchaseHistoryRecord* Record : Records)
		{
			const TArray<FString> ProductIds = Record->GetSkus();

			if (!ProductIds.IsEmpty() && ProductIds[0] == "removeads")
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Restoring Purchase: Remove Ads"));
				UserProgression.bNoAds = true;
				break;
			}
		}
	}
}