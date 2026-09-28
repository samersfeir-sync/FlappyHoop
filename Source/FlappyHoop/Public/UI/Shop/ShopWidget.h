// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/Common/PopupWidget.h"
#include "Data/BallsShopStruct.h"
#include "Data/BallCatalog.h"
#include "Data/GemShopInfo.h"
#include "ShopWidget.generated.h"

class UButton;
class UUniformGridPanel;
class UScrollBox;
class UTotalCoinsWidget;
class UTotalGemsWidget;
class UShopItemWidget;
class UGemShopWidget;
class UInsufficientCoinsWidget;

UCLASS()
class FLAPPYHOOP_API UShopWidget : public UPopupWidget
{
	GENERATED_BODY()

public:

	UFUNCTION()
	void GemButtonClicked();

	const TArray<TObjectPtr<UShopItemWidget>>& GetShopItemWidgets() const { return ShopItemWidgets; }

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTotalCoinsWidget> TotalCoinsWidget;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTotalGemsWidget> TotalGemsWidget;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UInsufficientCoinsWidget> InsufficientCoinsWidget;

protected:

	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void OnPopupOpening() override;
	virtual void OnPopupClosed() override;

private:

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BackButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BallsButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> GemsButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUniformGridPanel> ItemsGridPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUniformGridPanel> GemsGridPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> ScrollBox;

	UPROPERTY(EditDefaultsOnly, Category = "Shop")
	TSubclassOf<UShopItemWidget> ShopItemWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Shop")
	TSubclassOf<UGemShopWidget> GemShopWidgetClass;

	/** Designer preview only: the widget editor has no game instance, so show this catalog there. */
	UPROPERTY(EditDefaultsOnly, Category = "Shop")
	TObjectPtr<UBallCatalog> PreviewCatalog;

	/** OLD designer-preview list, used when PreviewCatalog is empty. */
	UPROPERTY(EditDefaultsOnly, Category = "Shop")
	TArray<FBallsShopStruct> BallShopItems;

	UPROPERTY(EditDefaultsOnly, Category = "Shop")
	TArray<FGemShopInfo> GemShopInfo;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UGemShopWidget>> GemShopWidgets;

	/** Localized prices arrived from Google Play. */
	void OnStorePricesUpdated();

	/** UPROPERTY so GC keeps them alive; reset on every refill (it used to grow forever with stale entries). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UShopItemWidget>> ShopItemWidgets;

	/** Rebuilds the ball tiles from the catalog (or the preview data in the designer). */
	void FillItemContainer();
	void AddBallTile(const FBallCatalogEntry& Entry, bool bOwned, int32 Index);

	void FillGemsContainer();
	void RefreshTotals();

	UFUNCTION()
	void HideShopWidget();

	UFUNCTION()
	void BallButtonClicked();

	void OnBalanceChanged(int32 Coins, int32 Gems);

};
