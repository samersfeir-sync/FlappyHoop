// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InsufficientCoinsWidget.generated.h"

class UButton;
class UTextBlock;
class UShopItemWidget;

UCLASS()
class FLAPPYHOOP_API UInsufficientCoinsWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:

	void SetInsufficientCoinsText(int32 GemsCost);
	void SetGemsAmountText(int32 GemsAmount);
	void SetGemsNeeded(int32 NewGemsNeeded) { GemsNeeded = NewGemsNeeded; }
	void SetShopItemWidget(UShopItemWidget* NewShopItemWidget) { ShopItemWidget = NewShopItemWidget; }

private:

	UPROPERTY(meta = (BindWidget))
	UButton* CloseButton;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* InsufficientCoinsText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* GemsAmountText;

	UPROPERTY(meta = (BindWidget))
	UButton* GemsButton;

	UFUNCTION()
	void OnCloseButtonClicked();

	UFUNCTION()
	void OnGemsButtonClicked();
	
	virtual void NativeConstruct() override;

	class IGameInstanceInterface* GameInstanceInterface = nullptr;

	int32 GemsNeeded = 0;
	
	UShopItemWidget* ShopItemWidget = nullptr;
};
