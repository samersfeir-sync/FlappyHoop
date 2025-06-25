// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShopItemWidget.generated.h"

class UTextBlock;
class UImage;
class UBorder;
class UButton;

UCLASS()
class FLAPPYHOOP_API UShopItemWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:

	void SetItemImage(UTexture2D* ItemTexture);
	void SetItemPriceText(const FString& PriceText, bool AdjustFont);
	void SetCoinImageVisibility(bool Visible);

private:

	UPROPERTY(meta = (BindWidget))
	UTextBlock* ItemPriceText;

	UPROPERTY(meta = (BindWidget))
	UImage* ItemImage;

	UPROPERTY(meta = (BindWidget))
	UBorder* MainBorder;

	UPROPERTY(meta = (BindWidget))
	UImage* CoinImage;

	UPROPERTY(meta = (BindWidget))
	UButton* BuyButton;
};
