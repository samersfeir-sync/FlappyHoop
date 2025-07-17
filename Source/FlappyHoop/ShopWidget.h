// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BallsShopStruct.h"
#include "GemShopInfo.h"
#include "MGAndroidBillingClient.h"
#include "MGAndroidBillingLibrary.h"
#include "MGAndroidBillingResult.h"
#include "ShopWidget.generated.h"

class UButton;
class UUniformGridPanel;
class UTotalCoinsWidget;
class UTotalGemsWidget;
class IGameModeInterface;
class UShopItemWidget;
class UInsufficientCoinsWidget;

DECLARE_DELEGATE(FOnAdsRemoved);

UCLASS()
class FLAPPYHOOP_API UShopWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:

	TArray<UShopItemWidget*> ShopItemWidgets;

	UPROPERTY(meta = (BindWidget))
	UTotalCoinsWidget* TotalCoinsWidget;

	UPROPERTY(meta = (BindWidget))
	UTotalGemsWidget* TotalGemsWidget;

	UPROPERTY(meta = (BindWidget))
	UInsufficientCoinsWidget* InsufficientCoinsWidget;

	void QueryProductDetails(FString ProductId);

	UFUNCTION()
	void GemButtonClicked();

	FOnAdsRemoved AdsRemovedDelegate;

private:

	virtual void NativeConstruct() override;

	virtual void NativePreConstruct() override;

	UPROPERTY(meta = (BindWidget))
	UButton* BackButton;

	UPROPERTY(meta = (BindWidget))
	UUniformGridPanel* ItemsGridPanel;

	UPROPERTY(meta = (BindWidget))
	UUniformGridPanel* GemsGridPanel;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class UShopItemWidget> ShopItemWidgetClass;

	UFUNCTION()
	void FillItemContainer(TArray<FBallsShopStruct> BallsShopStruct);

	UFUNCTION()
	void HideShopWidget();

	class IGameInstanceInterface* GameInstanceInterface = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TArray<FBallsShopStruct> BallShopItems;

	UPROPERTY(meta = (BindWidget))
	class UScrollBox* ScrollBox;

	IGameModeInterface* GameModeInterface = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TArray<FGemShopInfo> GemShopInfo;

	UPROPERTY(meta = (BindWidget))
	UButton* BallsButton;

	UPROPERTY(meta = (BindWidget))
	UButton* GemsButton;

	void FillGemsContainer();

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class UGemShopWidget> GemShopWidgetClass;

	UFUNCTION()
	void BallButtonClicked();

	void PurchaseSuccess();

	FAndroidPurchasesDelegate OnQueryPurchasesCompletedDelegate;

	FAndroidProductDetailsDelegate OnAndroidProductDetailsDelegate;

	UFUNCTION()
	void OnQueryProductDetailsCompleted(UMGAndroidBillingResult* Result, const TArray<UMGAndroidProductDetails*>& ProductDetails);

	FAndroidBillingResultDelegate OnBillingFlowStartedDelegate;

	TArray<FString> ProductIds;

	FAndroidConsumeResultDelegate OnCosumeDelegate;

	UFUNCTION()
	void GemsConsumed(UMGAndroidBillingResult* Result, const FString& Token);

	const FGemShopInfo* ProductFound;

	static const TCHAR* BillingResponseCodeToString(EMGAndroidBillingResponseCode Code);

	UFUNCTION()
	void CheckAndPrintConnectionAndBillingStatus();

	void QueryPurchases();
};
