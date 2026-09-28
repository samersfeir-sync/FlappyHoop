// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GemShopWidget.generated.h"

class UButton;
class UTextBlock;
class UImage;
class UTexture2D;
class UShopWidget;

UCLASS()
class FLAPPYHOOP_API UGemShopWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/** Fallback price from the shop widget's Gem Shop Info (shown as "$ <price>" until Google Play answers). */
	void SetGemPrice(const FString& Price);

	/** Shows Google Play's localized price if the store has it, otherwise the fallback. */
	void RefreshPrice();
	void SetGemAmount(int32 Amount);
	void SetGemImage(UTexture2D* Image);
	void SetProductId(const FString& Id) { ProductId = Id; }
	void SetParentWidget(UShopWidget* Parent) { ParentWidget = Parent; }

protected:

	virtual void NativeConstruct() override;

private:

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BuyButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> GemPrice;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> GemAmount;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> GemImage;

	UFUNCTION()
	void OnBuyButtonClicked();

	UPROPERTY(Transient)
	TObjectPtr<UShopWidget> ParentWidget = nullptr;

	FString ProductId;
	FString FallbackPrice;
};
