// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Shop/GemShopWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "UI/Shop/ShopWidget.h"
#include "Systems/StoreSubsystem.h"

void UGemShopWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BuyButton->OnClicked.AddUniqueDynamic(this, &UGemShopWidget::OnBuyButtonClicked);
}

void UGemShopWidget::SetGemPrice(const FString& Price)
{
	FallbackPrice = Price;
	RefreshPrice();
}

void UGemShopWidget::RefreshPrice()
{
	// No store in the widget designer / on desktop: the fallback is used.
	const UStoreSubsystem* Store = IsDesignTime() ? nullptr : UStoreSubsystem::Get(this);
	const FString Localized = Store ? Store->GetLocalizedPrice(ProductId) : FString();

	GemPrice->SetText(FText::FromString(Localized.IsEmpty() ? FString::Printf(TEXT("$ %s"), *FallbackPrice) : Localized));
}

void UGemShopWidget::SetGemAmount(int32 Amount)
{
	GemAmount->SetText(FText::AsNumber(Amount));
}

void UGemShopWidget::SetGemImage(UTexture2D* Image)
{
	GemImage->SetBrushFromTexture(Image);
}

void UGemShopWidget::OnBuyButtonClicked()
{
	if (UStoreSubsystem* Store = UStoreSubsystem::Get(this))
	{
		Store->Purchase(ProductId);
	}
}
