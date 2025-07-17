// Fill out your copyright notice in the Description page of Project Settings.


#include "ShopWidget.h"
#include "Components/UniformGridPanel.h"
#include "GameInstanceInterface.h"
#include "FunctionsLibrary.h"
#include "BallsShopStruct.h"
#include "ShopItemWidget.h"
#include "TotalCoinsWidget.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "TotalGemsWidget.h"
#include "GemShopWidget.h"
#include "MGAndroidBillingLibrary.h"
#include "MGAndroidBillingResult.h"
#include "MGAndroidPurchase.h"
#include "MGAndroidBillingFlowParameters.h"
#include "MGAndroidWrapperObject.h"
#include "MGAndroidProductDetails.h"
#include "InsufficientCoinsWidget.h"
#include "GameModeInterface.h"

void UShopWidget::NativeConstruct()
{
    Super::NativeConstruct();

    GameInstanceInterface = UFunctionsLibrary::GetGameInstanceInterface(this);
    GameModeInterface = UFunctionsLibrary::GetGameModeInterface(this);

    TArray<FBallsShopStruct> BallsStruct = GameInstanceInterface->GetUserProgression().BallsOwned;
    FillItemContainer(BallsStruct);
    BackButton->OnClicked.AddDynamic(this, &UShopWidget::HideShopWidget);
	BallsButton->OnClicked.AddDynamic(this, &UShopWidget::BallButtonClicked);
	GemsButton->OnClicked.AddDynamic(this, &UShopWidget::GemButtonClicked);

    InsufficientCoinsWidget->SetShopWidget(this);

#if PLATFORM_ANDROID

    QueryPurchases();
	GameInstanceInterface->GetOnPurchaseSuccessfulDelegate().BindUObject(this, &UShopWidget::PurchaseSuccess);

    //FTimerHandle BillingClientCheckTimer;
	//GetWorld()->GetTimerManager().SetTimer(BillingClientCheckTimer, this, &UShopWidget::CheckAndPrintConnectionAndBillingStatus, 1.0f, true);

#endif
}

void UShopWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

    if (IsDesignTime())
    {
        FillItemContainer(BallShopItems);
    }

    FillGemsContainer();
}

void UShopWidget::FillItemContainer(TArray<FBallsShopStruct> BallsShopStruct)
{
    ItemsGridPanel->ClearChildren();
    const int ItemsPerRow = 3;

    for (int32 i = 0; i < BallsShopStruct.Num(); ++i)
    {
        const FBallsShopStruct& BallItem = BallsShopStruct[i];

        UShopItemWidget* NewItem = CreateWidget<UShopItemWidget>(this, ShopItemWidgetClass);

        if (NewItem)
        {
            bool bIsPurchased = BallItem.IsPurchased;
            FString FormattedPrice = bIsPurchased ? "OWNED" : FString::FromInt(BallItem.Price);

            ShopItemWidgets.Add(NewItem);
            NewItem->SetParentWidgetReference(this);
            NewItem->SetGameInstanceInterface(GameInstanceInterface);
            NewItem->SetGameModeInterface(GameModeInterface);
            NewItem->SetBallShopStruct(BallItem);
            NewItem->SetItemImage(BallItem.BallTexture);
            NewItem->SetCoinImageVisibility(!bIsPurchased);
            NewItem->SetItemPriceText(FormattedPrice, bIsPurchased);
            NewItem->OnBallPurchased.AddDynamic(this, &UShopWidget::FillItemContainer);

            int32 Row = i / ItemsPerRow;
            int32 Column = i % ItemsPerRow;
            ItemsGridPanel->AddChildToUniformGrid(NewItem, Row, Column);
        }
    }

    if (IsDesignTime())
        return;

    int32 TotalCoins = GameInstanceInterface->GetUserProgression().TotalCoins;
    TotalCoinsWidget->UpdateCoinsText(TotalCoins);
	int32 TotalGems = GameInstanceInterface->GetUserProgression().TotalGems;
	TotalGemsWidget->UpdateGemsText(TotalGems);
}

void UShopWidget::HideShopWidget()
{
    SetVisibility(ESlateVisibility::Hidden);
    ScrollBox->ScrollToStart();

#if PLATFORM_ANDROID
    if (GameInstanceInterface->IsBillingClientReady())
    {
        ProductIds.Empty();
        ProductFound = nullptr;
        GameInstanceInterface->GetCurrentPurchase() = nullptr;
        GameInstanceInterface->GetPurchaseToken().Empty();
    }
#endif
}

void UShopWidget::FillGemsContainer()
{
    if (GemShopInfo.IsEmpty())
        return;

    const int GemsPerRow = 2;

    for (int32 i = 0; i < GemShopInfo.Num(); ++i)
    {
		UGemShopWidget* NewGemItem = CreateWidget<UGemShopWidget>(this, GemShopWidgetClass);

        if (NewGemItem)
        {
			NewGemItem->SetParentWidget(this);
			NewGemItem->SetGemPrice(GemShopInfo[i].Price);
			NewGemItem->SetGemAmount(GemShopInfo[i].Amount);
			NewGemItem->SetGemImage(GemShopInfo[i].Image);
			NewGemItem->SetProductId(GemShopInfo[i].ProductId);

            int32 Row = i / GemsPerRow;
            int32 Column = i % GemsPerRow;
            GemsGridPanel->AddChildToUniformGrid(NewGemItem, Row, Column);
        }
	}
}

void UShopWidget::BallButtonClicked()
{
    ItemsGridPanel->SetVisibility(ESlateVisibility::Visible);
    GemsGridPanel->SetVisibility(ESlateVisibility::Collapsed);
    BallsButton->SetIsEnabled(false);
	GemsButton->SetIsEnabled(true);
    ScrollBox->ScrollToStart();
}

void UShopWidget::GemButtonClicked()
{
    ItemsGridPanel->SetVisibility(ESlateVisibility::Collapsed);
    GemsGridPanel->SetVisibility(ESlateVisibility::Visible);
    BallsButton->SetIsEnabled(true);
    GemsButton->SetIsEnabled(false);
	ScrollBox->ScrollToStart();
}

const TCHAR* UShopWidget::BillingResponseCodeToString(EMGAndroidBillingResponseCode Code)
{
    switch (Code)
    {
    case EMGAndroidBillingResponseCode::Ok: return TEXT("Ok");
    case EMGAndroidBillingResponseCode::UserCancelled: return TEXT("UserCancelled");
    case EMGAndroidBillingResponseCode::ServiceUnavailable: return TEXT("ServiceUnavailable");
    case EMGAndroidBillingResponseCode::BillingUnavailable: return TEXT("BillingUnavailable");
    case EMGAndroidBillingResponseCode::ItemUnavailable: return TEXT("ItemUnavailable");
    case EMGAndroidBillingResponseCode::DeveloperError: return TEXT("DeveloperError");
    case EMGAndroidBillingResponseCode::Error: return TEXT("Error");
    case EMGAndroidBillingResponseCode::ItemAlreadyOwned: return TEXT("ItemAlreadyOwned");
    case EMGAndroidBillingResponseCode::ItemNotOwned: return TEXT("ItemNotOwned");
    default: return TEXT("Unknown");
    }
}

void UShopWidget::CheckAndPrintConnectionAndBillingStatus()
{
#if PLATFORM_ANDROID
    const bool bReady = GameInstanceInterface->IsBillingClientReady();

    EMGConnectionState ConnectionState = GameInstanceInterface->GetAndroidBillingClient()->GetConnectionState();
    FString StateStr;
    switch (ConnectionState)
    {
    case EMGConnectionState::Disconnected:
        StateStr = TEXT("Disconnected");
        break;
    case EMGConnectionState::Connecting:
        StateStr = TEXT("Connecting");
        break;
    case EMGConnectionState::Connected:
        StateStr = TEXT("Connected");
        break;
    case EMGConnectionState::Closed:
        StateStr = TEXT("Closed");
        break;
    default:
        StateStr = TEXT("Unknown");
        break;
    }
    GEngine->AddOnScreenDebugMessage(-1, 5.0f, bReady ? FColor::Green : FColor::Red, 
        FString::Printf(TEXT("Billing client ready: %s, connection state: %s"), 
            bReady ? TEXT("TRUE") : TEXT("FALSE"), *StateStr)); 
#endif
}

void UShopWidget::QueryPurchases()
{
    if (GameInstanceInterface->IsBillingClientReady())
    {
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Querying Purchases..."));

        OnQueryPurchasesCompletedDelegate.BindDynamic(GameInstanceInterface, &IGameInstanceInterface::OnPurchaseUpdated);

        FString Message = FString::Printf(TEXT("OnQueryPurchasesCompletedDelegate IsBound? %s"),
            OnQueryPurchasesCompletedDelegate.IsBound() ? TEXT("true") : TEXT("false"));
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, Message);
        
        GameInstanceInterface->GetAndroidBillingClient()->QueryPurchases(EMGSkuType::Purchase, OnQueryPurchasesCompletedDelegate);
    }
}

void UShopWidget::PurchaseSuccess()
{
    if (ProductIds.IsEmpty())
        return;

    const FString ProductId = ProductIds[0];

    if (ProductId == "removeads")
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Remove ads product purchased."));
		FUserProgression UserProgression = GameInstanceInterface->GetUserProgression();
		UserProgression.bNoAds = true;
		GameInstanceInterface->SaveUserProgression(UserProgression);

        if (GameInstanceInterface->GetBannerAdInterface())
        {
            GameInstanceInterface->GetBannerAdInterface()->Destroy();
        }

		GameModeInterface->StopInterstitialTimer();
		AdsRemovedDelegate.ExecuteIfBound();
        return;
    }

    ProductFound = GemShopInfo.FindByPredicate([&](const FGemShopInfo& Info)
        {
            return Info.ProductId == ProductId;
        });

    if (ProductFound)
    {
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Product Found: " + ProductFound->ProductId));
		OnCosumeDelegate.BindDynamic(this, &UShopWidget::GemsConsumed);
        GameInstanceInterface->GetAndroidBillingClient()->Consume(GameInstanceInterface->GetPurchaseToken(), OnCosumeDelegate);
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Gems product not found."));
    }
}

void UShopWidget::OnQueryProductDetailsCompleted(UMGAndroidBillingResult* Result, const TArray<UMGAndroidProductDetails*>& ProductDetails)
{
    if (UFunctionsLibrary::BillingResponseOK(Result))
    {
        if (ProductDetails.Num() > 0)
        {
            if (GameInstanceInterface->IsBillingClientReady())
            {
                UMGAndroidBillingFlowParameters* BillingFlowParams;
                BillingFlowParams = UMGAndroidBillingLibrary::CreateAndroidBillingFlowParameters();
				BillingFlowParams->SetProductDetailsParams(ProductDetails[0]);
                GameInstanceInterface->GetAndroidBillingClient()->LaunchBillingFlow(BillingFlowParams, OnBillingFlowStartedDelegate);
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Billing Flow Started for Product: " + ProductDetails[0]->GetProductId()));
            }
        }
    }
}

void UShopWidget::GemsConsumed(UMGAndroidBillingResult* Result, const FString& Token)
{
    if (UFunctionsLibrary::BillingResponseOK(Result))
    {
		FUserProgression UserProgression = GameInstanceInterface->GetUserProgression();
		UserProgression.TotalGems += ProductFound->Amount;
		GameInstanceInterface->SaveUserProgression(UserProgression);
		TotalGemsWidget->UpdateGemsText(UserProgression.TotalGems);
        GameInstanceInterface->GetPurchaseToken().Empty();
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Gems Consumed Successfully: " + Token));
    }
}

void UShopWidget::QueryProductDetails(FString ProductId)
{
    if (GameInstanceInterface->IsBillingClientReady())
    {
        ProductIds.SetNum(1);
        ProductIds[0] = ProductId;
		OnAndroidProductDetailsDelegate.BindDynamic(this, &UShopWidget::OnQueryProductDetailsCompleted);
        GameInstanceInterface->GetAndroidBillingClient()->QueryProductDetails(EMGSkuType::Purchase, ProductIds, OnAndroidProductDetailsDelegate);
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Querying Product Details for: " + ProductId));
    }
}
