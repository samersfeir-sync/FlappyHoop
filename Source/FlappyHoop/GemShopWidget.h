// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GemShopWidget.generated.h"

class UButton;
class UTextBlock;
class UImage;
class UShopWidget;

UCLASS()
class FLAPPYHOOP_API UGemShopWidget : public UUserWidget
{
	GENERATED_BODY()
	

public:

	void SetGemPrice(FString Price);
	void SetGemAmount(int32 Amount);
	void SetGemImage(UTexture2D* Image);
	void SetProductId(const FString& Id) { ProductId = Id; }
	void SetParentWidget(UShopWidget* Parent) { ParentWidget = Parent; }

private:

	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	UButton* BuyButton;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* GemPrice;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* GemAmount;

	UPROPERTY(meta = (BindWidget))
	UImage* GemImage;

	UFUNCTION()
	void OnBuyButtonClicked();

	UPROPERTY()
	UShopWidget* ParentWidget = nullptr;

	FString ProductId;
};
