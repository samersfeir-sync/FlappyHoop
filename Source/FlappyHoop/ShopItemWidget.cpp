// Fill out your copyright notice in the Description page of Project Settings.


#include "ShopItemWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Border.h"

void UShopItemWidget::SetItemImage(UTexture2D* ItemTexture)
{
	ItemImage->SetBrushFromTexture(ItemTexture, false);
}

void UShopItemWidget::SetItemPriceText(const FString& PriceText, bool AdjustFont)
{
	ItemPriceText->SetText(FText::FromString(PriceText));

	if (AdjustFont)
	{
		ItemPriceText->SetRenderTranslation(FVector2D(-54, 0));
		ItemPriceText->Font.Size = 35;
	}
}

void UShopItemWidget::SetCoinImageVisibility(bool Visible)
{
	ESlateVisibility SizeBoxVisibility = Visible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden;
	CoinImage->SetVisibility(SizeBoxVisibility);
}
