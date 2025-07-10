// Fill out your copyright notice in the Description page of Project Settings.


#include "GemShopWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

void UGemShopWidget::SetGemPrice(FString Price)
{
	FString FormattedPrice = FString::Printf(TEXT("$ %s"), *Price);
	GemPrice->SetText(FText::FromString(FormattedPrice));
}

void UGemShopWidget::SetGemAmount(int32 Amount)
{
	GemAmount->SetText(FText::AsNumber(Amount));
}

void UGemShopWidget::SetGemImage(UTexture2D* Image)
{
	GemImage->SetBrushFromTexture(Image);
}
