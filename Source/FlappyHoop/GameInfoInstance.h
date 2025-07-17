// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GameInstanceInterface.h"
#include "GameInfoInstance.generated.h"

class UMySaveGame;

UCLASS()
class FLAPPYHOOP_API UGameInfoInstance : public UGameInstance, public IGameInstanceInterface
{
	GENERATED_BODY()

public:

	virtual void SaveUserProgression(FUserProgression& NewUserProgression) override;
	virtual void LoadUserProgression() override;
	virtual FUserProgression& GetUserProgression() override;
	virtual FString GetBannerAdUnitID() const override { return BannerADUnitID; }
	virtual FString GetInterstitialAdUnitID() const override { return InterstitialADUnitID; }
	virtual FString GetRewardedAdUnitID() const override { return RewardedADUnitID; }
	virtual const TScriptInterface<IAGBannerAdInterface> GetBannerAdInterface() const { return BannerAdInterface; }
	virtual UMGAndroidBillingClient* GetAndroidBillingClient() const override { return AndroidBillingClient; }
	virtual bool IsBillingClientReady() const override;

	UFUNCTION()
	virtual void OnPurchaseUpdated(UMGAndroidBillingResult* Result, const TArray<UMGAndroidPurchase*>& Purchases) override;

	virtual FString& GetPurchaseToken() override { return PurchaseToken; }
	virtual FOnPurchaseSuccessful& GetOnPurchaseSuccessfulDelegate() override { return OnPurchaseSuccessfulDelegate; }
	virtual UMGAndroidPurchase*& GetCurrentPurchase() override { return CurrentPurchase; }

private:

	UPROPERTY()
	UMySaveGame* SaveGame = nullptr;

	FUserProgression UserProgression;

	virtual void Init() override;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UMySaveGame> SaveGameClass;

	void InitializeADUnits();

	FString BannerADUnitID;
	FString InterstitialADUnitID;

	FString RewardedADUnitID;

	void OnMoviePlaybackFinished();

	TScriptInterface<IAGBannerAdInterface> BannerAdInterface;

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	USoundBase* GameMusic;

	virtual void PlayBackgroundMusic() override;

	virtual void StopBackgroundMusic() override;

	UPROPERTY()
	UAudioComponent* MusicAudioComponent;

	UPROPERTY()
	UMGAndroidBillingClient* AndroidBillingClient = nullptr;

	FOnPurchasesUpdated OnPurchaseUpdatedDelegate;
	FAndroidBillingResultDelegate OnBillingSetupFinishedDelegate;
	FAndroidBillingVoidDelegate OnBillingDisconnectedDelegate;
	FAndroidPurchaseHistoryDelegate OnPurchaseHistoryFetchedDelegate;
	FAndroidBillingResultDelegate OnAcknowledgeCompletedDelegate;

	UFUNCTION()
	void RetryConnection();

	void StartConnection();

	UFUNCTION()
	void BillingSetupFinished(UMGAndroidBillingResult* Result);

	UFUNCTION()
	void RestorePurchases(UMGAndroidBillingResult* Result, const TArray<UMGAndroidPurchaseHistoryRecord*>& Records);

	UPROPERTY()
	UMGAndroidPurchase* CurrentPurchase;

	FString PurchaseToken = "";

	UFUNCTION()
	void OnAcknowledgeCompleted(UMGAndroidBillingResult* Result);

	FOnPurchaseSuccessful OnPurchaseSuccessfulDelegate;
};
