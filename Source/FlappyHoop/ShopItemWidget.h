// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BallsShopStruct.h"
#include "ShopItemWidget.generated.h"

class UTextBlock;
class UImage;
class UBorder;
class UButton;
class IGameInstanceInterface;
class IGameModeInterface;
class UShopWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBallPurchased, TArray<FBallsShopStruct>, BallsShopStruct);

UCLASS()
class FLAPPYHOOP_API UShopItemWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:

	void SetItemImage(UTexture2D* ItemTexture);
	void SetItemPriceText(const FString& PriceText, bool AdjustFont);
	void SetBallShopStruct(FBallsShopStruct NewBallShopStruct) { BallShopStruct = NewBallShopStruct; }
	void SetCoinImageVisibility(bool Visible);
	void SetGameInstanceInterface(IGameInstanceInterface* NewInterface) { GameInstanceInterface = NewInterface; }
	void SetGameModeInterface(IGameModeInterface* NewInterface) { GameModeInterface = NewInterface; }
	void SetParentWidgetReference(UShopWidget* NewParentWidget) { ParentWidget = NewParentWidget; }

	UFUNCTION()
	void OnBuyButtonClicked();

	FOnBallPurchased OnBallPurchased;

private:

	virtual void NativeConstruct() override;

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

	IGameInstanceInterface* GameInstanceInterface;
	IGameModeInterface* GameModeInterface;

	FBallsShopStruct BallShopStruct;

	void UpdateBorderColor();

	UShopWidget* ParentWidget;

	UPROPERTY(EditDefaultsOnly)
	FLinearColor SelectedOutlineColor;

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	USoundBase* PurchaseSound;
};
