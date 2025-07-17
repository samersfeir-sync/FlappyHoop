// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "UserProgression.h"
#include "Interface/AGBannerAdInterface.h"
#include "MGAndroidBillingClient.h"
#include "GameInstanceInterface.generated.h"

DECLARE_DELEGATE(FOnPurchaseSuccessful)

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UGameInstanceInterface : public UInterface
{
	GENERATED_BODY()
};

class FLAPPYHOOP_API IGameInstanceInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	virtual void SaveUserProgression(FUserProgression& NewUserProgression) = 0;
	virtual void LoadUserProgression() = 0;
	virtual FUserProgression& GetUserProgression() = 0;
	virtual FString GetBannerAdUnitID() const = 0;
	virtual FString GetInterstitialAdUnitID() const = 0;
	virtual FString GetRewardedAdUnitID() const = 0;
	virtual void PlayBackgroundMusic() = 0;
	virtual void StopBackgroundMusic() = 0;
	virtual const TScriptInterface<IAGBannerAdInterface> GetBannerAdInterface() const = 0;
	virtual UMGAndroidBillingClient* GetAndroidBillingClient() const = 0;
	virtual bool IsBillingClientReady() const = 0;
	virtual void OnPurchaseUpdated(UMGAndroidBillingResult* Result, const TArray<UMGAndroidPurchase*>& Purchases) = 0;
	virtual FString& GetPurchaseToken() = 0;
	virtual FOnPurchaseSuccessful& GetOnPurchaseSuccessfulDelegate() = 0;
	virtual UMGAndroidPurchase*& GetCurrentPurchase() = 0;
};
