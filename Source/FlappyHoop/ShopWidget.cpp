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

#if PLATFORM_ANDROID
    OnBillingSetupFinishedDelegate.BindDynamic(this, &UShopWidget::BillingSetupFinished);
    OnBillingDisconnectedDelegate.BindDynamic(this, &UShopWidget::StartConnection);
    OnPurchaseUpdatedDelegate.BindDynamic(this, &UShopWidget::OnPurchaseUpdated);

    AndroidBillingClient = UMGAndroidBillingLibrary::CreateAndroidBillingClient(OnPurchaseUpdatedDelegate);
    
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Created Android Billing Client"));

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
    if (CheckBillingClient())
    {
        AndroidBillingClient->EndConnection();
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Android Billing Client Connection Ended"));
        ProductIds.Empty();
        ProductFound = nullptr;
        CurrentPurchase = nullptr;
        PurchaseToken.Empty();
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

void UShopWidget::OnPurchaseUpdated(UMGAndroidBillingResult* Result, const TArray<UMGAndroidPurchase*>& Purchases)
{
    if (BillingResponseOK(Result))
    {
        for (UMGAndroidPurchase* Purchase : Purchases)
        {
            if(!Purchase->IsAcknowledged() && Purchase->GetPurchaseState() == EMGAndroidPurchaseState::Purchased)
            {
                if (CheckBillingClient())
                {
                    CurrentPurchase = Purchase;
					PurchaseToken = Purchase->GetPurchaseToken();
					OnAcknowledgeCompletedDelegate.BindDynamic(this, &UShopWidget::OnAcknowledgeCompleted);
                    AndroidBillingClient->AcknowledgePurchase(PurchaseToken, OnAcknowledgeCompletedDelegate);
                    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Acknowledging Purchase: %s"), *PurchaseToken));
                }
			}
        }
    }
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

bool UShopWidget::BillingResponseOK(UMGAndroidBillingResult* BillingResult) const
{
    const bool bResult = BillingResult && BillingResult->GetResponseCode() == EMGAndroidBillingResponseCode::Ok;

    FString Message;

    if (!BillingResult)
    {
        Message = TEXT("BillingResponseOK: BillingResult is NULL");
    }
    else
    {
        const auto Code = BillingResult->GetResponseCode();
        Message = FString::Printf(
            TEXT("BillingResponseOK: Response Code = %d (%s), Result = %s"),
            static_cast<int32>(Code),
            BillingResponseCodeToString(Code),
            bResult ? TEXT("TRUE") : TEXT("FALSE")
        );
    }

    GEngine->AddOnScreenDebugMessage(-1, 5.0f, bResult ? FColor::Green : FColor::Red, Message);

    return bResult;
}

bool UShopWidget::CheckBillingClient() const
{
#if PLATFORM_ANDROID
    const bool bValid = AndroidBillingClient != nullptr;
    const bool bNativeValid = bValid && AndroidBillingClient->IsNativeObjectValid();
    const bool bReady = bNativeValid && AndroidBillingClient->IsReady();

    FString Message = FString::Printf(
        TEXT("CheckBillingClient: Valid=%s, NativeValid=%s, Ready=%s, Result=%s"),
        bValid ? TEXT("TRUE") : TEXT("FALSE"),
        bNativeValid ? TEXT("TRUE") : TEXT("FALSE"),
        bReady ? TEXT("TRUE") : TEXT("FALSE"),
        bReady ? TEXT("TRUE") : TEXT("FALSE")
    );

    GEngine->AddOnScreenDebugMessage(-1, 5.0f, bReady ? FColor::Green : FColor::Red, Message);

    return bReady;
#else
    return false;
#endif
}

void UShopWidget::OnAcknowledgeCompleted(UMGAndroidBillingResult* Result)
{
    if (BillingResponseOK(Result))
    {
        if (CheckBillingClient() && CurrentPurchase)
        {
            EMGAndroidPurchaseState PurchaseState = CurrentPurchase->GetPurchaseState();

            switch (PurchaseState)
            {
            case EMGAndroidPurchaseState::Unspecified:
                GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Purchase State: Unspecified"));
                break;

            case EMGAndroidPurchaseState::Purchased:
                GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Purchase State: Purchased"));
                PurchaseSuccess();
                break;

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

void UShopWidget::PurchaseSuccess()
{
    if (ProductIds.IsEmpty())
        return;

    FString ProductId = ProductIds[0];

    ProductFound = GemShopInfo.FindByPredicate([&](const FGemShopInfo& Info)
        {
            return Info.ProductId == ProductId;
        });

    if (ProductFound)
    {
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Product Found: " + ProductFound->ProductId));
		OnCosumeDelegate.BindDynamic(this, &UShopWidget::GemsConsumed);
        AndroidBillingClient->Consume(PurchaseToken, OnCosumeDelegate);
    }
}

void UShopWidget::BillingSetupFinished(UMGAndroidBillingResult* Result)
{
    if (BillingResponseOK(Result))
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Billing Client Setup Finished Successfully"));
		OnAndroidPurchasesDelegate.BindDynamic(this, &UShopWidget::OnPurchaseUpdated);
        AndroidBillingClient->QueryPurchases(EMGSkuType::Purchase, OnAndroidPurchasesDelegate);
    }
}

void UShopWidget::OnQueryProductDetailsCompleted(UMGAndroidBillingResult* Result, const TArray<UMGAndroidProductDetails*>& ProductDetails)
{
    if (BillingResponseOK(Result))
    {
        if (ProductDetails.Num() > 0)
        {
            if (CheckBillingClient())
            {
                UMGAndroidBillingFlowParameters* BillingFlowParams;
                BillingFlowParams = UMGAndroidBillingLibrary::CreateAndroidBillingFlowParameters();
				BillingFlowParams->SetProductDetailsParams(ProductDetails[0]);
                AndroidBillingClient->LaunchBillingFlow(BillingFlowParams, OnBillingFlowStartedDelegate);
				GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Billing Flow Started for Product: " + ProductDetails[0]->GetProductId()));
            }
        }
    }
}

void UShopWidget::GemsConsumed(UMGAndroidBillingResult* Result, const FString& Token)
{
    if (BillingResponseOK(Result))
    {
		FUserProgression UserProgression = GameInstanceInterface->GetUserProgression();
		UserProgression.TotalGems += ProductFound->Amount;
		GameInstanceInterface->SaveUserProgression(UserProgression);
		TotalGemsWidget->UpdateGemsText(UserProgression.TotalGems);
        PurchaseToken.Empty();
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Gems Consumed Successfully: " + Token));
    }
}

void UShopWidget::StartConnection()
{
    if (CheckBillingClient())
    {
        AndroidBillingClient->StartConnection(OnBillingSetupFinishedDelegate, OnBillingDisconnectedDelegate);
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Starting Android Billing Client Connection"));
    }
}

void UShopWidget::QueryProductDetails(FString ProductId)
{
    if (CheckBillingClient())
    {
        ProductIds.Add(ProductId);
		OnAndroidProductDetailsDelegate.BindDynamic(this, &UShopWidget::OnQueryProductDetailsCompleted);
        AndroidBillingClient->QueryProductDetails(EMGSkuType::Purchase, ProductIds, OnAndroidProductDetailsDelegate);
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Querying Product Details for: " + ProductId));
    }
}
