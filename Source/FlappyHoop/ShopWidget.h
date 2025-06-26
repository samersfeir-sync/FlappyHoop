// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BallsShopStruct.h"
#include "ShopWidget.generated.h"

class UButton;
class UUniformGridPanel;
class UTotalCoinsWidget;
class IGameModeInterface;
class UShopItemWidget;

UCLASS()
class FLAPPYHOOP_API UShopWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:

	TArray<UShopItemWidget*> ShopItemWidgets;

	UPROPERTY(meta = (BindWidget))
	UTotalCoinsWidget* TotalCoinsWidget;

private:

	virtual void NativeConstruct() override;

	//virtual void NativePreConstruct() override;

	UPROPERTY(meta = (BindWidget))
	UButton* BackButton;

	UPROPERTY(meta = (BindWidget))
	UUniformGridPanel* ItemsGridPanel;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class UShopItemWidget> ShopItemWidgetClass;

	UFUNCTION()
	void FillItemContainer(TArray<FBallsShopStruct> BallsShopStruct);

	UFUNCTION()
	void HideShopWidget();

	class IGameInstanceInterface* GameInstanceInterface = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TArray<FBallsShopStruct> BallShopItems;

	UPROPERTY(meta = (BindWidget))
	class UScrollBox* ScrollBox;

	IGameModeInterface* GameModeInterface = nullptr;
};
