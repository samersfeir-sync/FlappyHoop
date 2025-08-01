// Fill out your copyright notice in the Description page of Project Settings.


#include "ShopItemWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "GameInstanceInterface.h"
#include "UserProgression.h"
#include "GameModeInterface.h"
#include "ShopWidget.h"
#include "Kismet/GameplayStatics.h"
#include "InsufficientCoinsWidget.h"

void UShopItemWidget::SetItemImage(UTexture2D* ItemTexture)
{
	ItemImage->SetBrushFromTexture(ItemTexture, false);
}

void UShopItemWidget::SetItemPriceText(const FString& PriceText, bool AdjustFont)
{
	ItemPriceText->SetText(FText::FromString(PriceText));

	if (AdjustFont)
	{
		ItemPriceText->SetRenderTranslation(FVector2D(-59, 0));
		ItemPriceText->Font.Size = 35;
	}
}

void UShopItemWidget::SetCoinImageVisibility(bool Visible)
{
	ESlateVisibility SizeBoxVisibility = Visible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden;
	CoinImage->SetVisibility(SizeBoxVisibility);
}

void UShopItemWidget::PurchaseItem(FUserProgression UserProgression, int BallIndex, int32 GemsUsed)
{
	UserProgression.BallsOwned[BallIndex].IsPurchased = true;
	UserProgression.TotalCoins = FMath::Max(0, UserProgression.TotalCoins - BallShopStruct.Price);
	UserProgression.TotalGems = FMath::Max(0, UserProgression.TotalGems - GemsUsed);
	UserProgression.BallType = BallShopStruct.BallType;
	BallShopStruct.IsPurchased = true;
	GameModeInterface->SetBallType(BallShopStruct.BallType);
	GameModeInterface->ApplyBallSettings();
	GameInstanceInterface->SaveUserProgression(UserProgression);
	OnBallPurchased.Broadcast(UserProgression.BallsOwned);
	UGameplayStatics::PlaySound2D(this, PurchaseSound);
}

void UShopItemWidget::OnBuyButtonClicked()
{
	if (GameInstanceInterface)
	{
		FUserProgression UserProgression = GameInstanceInterface->GetUserProgression();

		if (!BallShopStruct.IsPurchased)
		{
			IndexFound = UserProgression.BallsOwned.Find(BallShopStruct);

			if (IndexFound != INDEX_NONE)
			{
				if (UserProgression.TotalCoins >= BallShopStruct.Price)
				{
					PurchaseItem(UserProgression, IndexFound, 0);
				}

				else
				{
					UInsufficientCoinsWidget* InsufficientCoinsWidget = ParentWidget->InsufficientCoinsWidget;

					int32 GemsNeeded = FMath::RoundToInt((BallShopStruct.Price - UserProgression.TotalCoins) * GemsNeededMultiplier);
					InsufficientCoinsWidget->SetGemsNeeded(GemsNeeded);
					InsufficientCoinsWidget->SetShopItemWidget(this);
					InsufficientCoinsWidget->SetInsufficientCoinsText(GemsNeeded);
					InsufficientCoinsWidget->SetGemsAmountText(GemsNeeded);
					InsufficientCoinsWidget->SetVisibility(ESlateVisibility::Visible);
				}
			}
		}

		else
		{
			UserProgression.BallType = BallShopStruct.BallType;
			GameInstanceInterface->SaveUserProgression(UserProgression);
			GameModeInterface->SetBallType(BallShopStruct.BallType);
			GameModeInterface->ApplyBallSettings();
			UpdateBorderColor();
		}
	}
}

void UShopItemWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BuyButton->OnClicked.AddDynamic(this, &UShopItemWidget::OnBuyButtonClicked);
	UpdateBorderColor();
}

void UShopItemWidget::UpdateBorderColor()
{
	const FLinearColor DefaultOutlineColor(0.38f, 0.15f, 0.0f, 1.0f);

	EBallType SelectedBallType = GameModeInterface->GetBallType();

	for (UShopItemWidget* ShopItem : ParentWidget->ShopItemWidgets)
	{
		FLinearColor DesiredColor = (ShopItem->BallShopStruct.BallType == SelectedBallType) ? SelectedOutlineColor : DefaultOutlineColor;
		FSlateBrush ShopItemBrush = ShopItem->MainBorder->Background;
		ShopItemBrush.OutlineSettings.Color = DesiredColor;
		ShopItem->MainBorder->SetBrush(ShopItemBrush);
	}
}