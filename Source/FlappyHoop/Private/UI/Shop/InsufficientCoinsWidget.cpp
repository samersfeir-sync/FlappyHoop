// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Shop/InsufficientCoinsWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Data/UserProgression.h"
#include "Systems/EconomySubsystem.h"
#include "UI/Shop/ShopItemWidget.h"
#include "UI/Shop/ShopWidget.h"

void UInsufficientCoinsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CloseButton->OnClicked.AddUniqueDynamic(this, &UInsufficientCoinsWidget::OnCloseButtonClicked);
	GemsButton->OnClicked.AddUniqueDynamic(this, &UInsufficientCoinsWidget::OnGemsButtonClicked);
}

void UInsufficientCoinsWidget::SetInsufficientCoinsText(int32 GemCost)
{
	if (InsufficientCoinsText)
	{
		InsufficientCoinsText->SetText(FText::FromString(
			FString::Printf(TEXT("You don't have enough coins. Use %d gems to complete the purchase?"), GemCost)));
	}
}

void UInsufficientCoinsWidget::SetGemsAmountText(int32 GemsAmount)
{
	if (GemsAmountText)
	{
		GemsAmountText->SetText(FText::FromString(FString::Printf(TEXT("%04d"), GemsAmount)));
	}
}

void UInsufficientCoinsWidget::OnGemsButtonClicked()
{
	if (!IsOpen())
	{
		return; // still shrinking from a previous tap
	}

	const UEconomySubsystem* Economy = UEconomySubsystem::Get(this);
	const bool bCanAfford = Economy && Economy->GetGems() >= GemsNeeded;

	UShopItemWidget* ItemToBuy = ShopItemWidget;
	UShopWidget* Shop = ShopWidget;
	const int32 Cost = GemsNeeded;

	Close();

	if (bCanAfford && ItemToBuy)
	{
		ItemToBuy->PurchaseItem(Cost);
	}
	else if (Shop)
	{
		Shop->GemButtonClicked(); // not enough gems: jump to the gem store tab
	}
}

void UInsufficientCoinsWidget::OnCloseButtonClicked()
{
	Close();
}

void UInsufficientCoinsWidget::OnPopupClosed()
{
	GemsNeeded = 0;
	ShopItemWidget = nullptr;
}
