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

void UShopWidget::NativeConstruct()
{
    Super::NativeConstruct();

    GameInstanceInterface = UFunctionsLibrary::GetGameInstanceInterface(this);
    FillItemContainer(GameInstanceInterface->GetUserProgression().BallsOwned);

    BackButton->OnClicked.AddDynamic(this, &UShopWidget::HideShopWidget);
}

void UShopWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

    if (IsDesignTime())
    {
        FillItemContainer(BallShopItems);
    }
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
            ShopItemWidgets.Add(NewItem);
            // NewItem->SetParentWidgetReference(this);

           //  NewItem->OnBallPurchased.AddDynamic(this, &UShopScreenWidget::FillItemContainer);
            // NewItem->SetGameInstanceInterface(GameInstanceInterface);

            bool bIsPurchased = BallItem.IsPurchased;
            NewItem->SetItemImage(BallItem.BallTexture);
            NewItem->SetCoinImageVisibility(!bIsPurchased);
            // NewItem->SetBallType(BallItem.BallType);
            FString FormattedPrice = bIsPurchased ? "OWNED" : FString::FromInt(BallItem.Price);
            NewItem->SetItemPriceText(FormattedPrice, bIsPurchased);

            int32 Row = i / ItemsPerRow;
            int32 Column = i % ItemsPerRow;
            ItemsGridPanel->AddChildToUniformGrid(NewItem, Row, Column);
        }
    }

    if (TotalCoinsWidget && GameInstanceInterface)
    {
        TotalCoinsWidget->UpdateCoinsText(GameInstanceInterface->GetUserProgression().TotalCoins);
    }
}

void UShopWidget::HideShopWidget()
{
    SetVisibility(ESlateVisibility::Hidden);
    ScrollBox->ScrollToStart();
}
