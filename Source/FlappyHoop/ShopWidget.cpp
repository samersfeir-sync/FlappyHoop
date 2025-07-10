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
			NewGemItem->SetGemPrice(GemShopInfo[i].Price);
			NewGemItem->SetGemAmount(GemShopInfo[i].Amount);
			NewGemItem->SetGemImage(GemShopInfo[i].Image);

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