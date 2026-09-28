// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/BallCatalog.h"
#include "ShopItemWidget.generated.h"

class UTextBlock;
class UImage;
class UBorder;
class UButton;
class USoundBase;
class UShopWidget;

/** One ball tile in the shop. Buy it (coins, or coins + gems via the insufficient-coins popup) or select it. */
UCLASS()
class FLAPPYHOOP_API UShopItemWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/** Fills the tile for a ball. */
	void SetEntry(const FBallCatalogEntry& NewEntry, bool bNewOwned);

	void SetParentWidgetReference(UShopWidget* NewParentWidget) { ParentWidget = NewParentWidget; }

	EBallType GetBallType() const { return Entry.BallType; }

	/** Buys this ball using all needed coins plus GemsUsed gems (0 when paying with coins only). */
	void PurchaseItem(int32 GemsUsed);

	/** Highlights the tile if its ball is the selected one. */
	void UpdateBorderColor(EBallType SelectedBallType);

	UFUNCTION()
	void OnBuyButtonClicked();

protected:

	virtual void NativeConstruct() override;

private:

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ItemPriceText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ItemImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> MainBorder;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> CoinImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BuyButton;

	UPROPERTY(Transient)
	FBallCatalogEntry Entry;
	bool bOwned = false;

	void SetItemPriceText(const FString& PriceText, bool bOwnedStyle);

	UPROPERTY(Transient)
	TObjectPtr<UShopWidget> ParentWidget = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Style")
	FLinearColor SelectedOutlineColor = FLinearColor::Transparent; // matches the old zero-initialised default; set in the BP

	UPROPERTY(EditDefaultsOnly, Category = "Style")
	FLinearColor DefaultOutlineColor = FLinearColor(0.38f, 0.15f, 0.0f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	TObjectPtr<USoundBase> PurchaseSound;

	/** Gems charged per missing coin when the player can't afford a ball. */
	UPROPERTY(EditDefaultsOnly, Category = "Shop")
	float GemsNeededMultiplier = 2.0f;
};
