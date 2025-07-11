// Fill out your copyright notice in the Description page of Project Settings.


#include "GemShopWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "ShopWidget.h"

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

void UGemShopWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BuyButton->OnClicked.AddDynamic(this, &UGemShopWidget::OnBuyButtonClicked);	
}

void UGemShopWidget::OnBuyButtonClicked()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Buy button clicked, product ID: %s"), *ProductId));

#if PLATFORM_ANDROID
	ParentWidget->QueryProductDetails(ProductId);
#endif
}