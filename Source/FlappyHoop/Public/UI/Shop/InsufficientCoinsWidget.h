// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/Common/PopupWidget.h"
#include "InsufficientCoinsWidget.generated.h"

class UButton;
class UTextBlock;
class UShopItemWidget;
class UShopWidget;

UCLASS()
class FLAPPYHOOP_API UInsufficientCoinsWidget : public UPopupWidget
{
	GENERATED_BODY()

public:

	void SetInsufficientCoinsText(int32 GemsCost);
	void SetGemsAmountText(int32 GemsAmount);
	void SetGemsNeeded(int32 NewGemsNeeded) { GemsNeeded = NewGemsNeeded; }
	void SetShopItemWidget(UShopItemWidget* NewShopItemWidget) { ShopItemWidget = NewShopItemWidget; }
	void SetShopWidget(UShopWidget* NewShopWidget) { ShopWidget = NewShopWidget; }

protected:

	virtual void NativeConstruct() override;
	virtual void OnPopupClosed() override;

private:

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CloseButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> InsufficientCoinsText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> GemsAmountText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> GemsButton;

	UFUNCTION()
	void OnCloseButtonClicked();

	UFUNCTION()
	void OnGemsButtonClicked();

	int32 GemsNeeded = 0;

	UPROPERTY(Transient)
	TObjectPtr<UShopItemWidget> ShopItemWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UShopWidget> ShopWidget = nullptr;
};
