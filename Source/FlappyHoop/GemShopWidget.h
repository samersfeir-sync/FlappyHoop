// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GemShopWidget.generated.h"

class UButton;
class UTextBlock;
class UImage;

UCLASS()
class FLAPPYHOOP_API UGemShopWidget : public UUserWidget
{
	GENERATED_BODY()
	

public:

	void SetGemPrice(FString Price);
	void SetGemAmount(int32 Amount);
	void SetGemImage(UTexture2D* Image);

private:

	UPROPERTY(meta = (BindWidget))
	UButton* BuyButton;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* GemPrice;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* GemAmount;

	UPROPERTY(meta = (BindWidget))
	UImage* GemImage;
};
