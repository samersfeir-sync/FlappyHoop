// Fill out your copyright notice in the Description page of Project Settings.


#include "InsufficientCoinsWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "UserProgression.h"
#include "GameInstanceInterface.h"
#include "FunctionsLibrary.h"
#include "ShopItemWidget.h"
#include "ShopWidget.h"

void UInsufficientCoinsWidget::SetInsufficientCoinsText(int32 GemCost)
{
	if (InsufficientCoinsText)
	{
		FString Message = FString::Printf(TEXT("You don't have enough coins. Use %d gems to complete the purchase?"), GemCost);
		InsufficientCoinsText->SetText(FText::FromString(Message));
	}
}

void UInsufficientCoinsWidget::SetGemsAmountText(int32 GemsAmount)
{
	FString FormattedText = FString::Printf(TEXT("%04d"), GemsAmount);
	GemsAmountText->SetText(FText::FromString(FormattedText));
}

void UInsufficientCoinsWidget::OnGemsButtonClicked()
{
	FUserProgression UserProgression = GameInstanceInterface->GetUserProgression();
	int32 TotalGems = UserProgression.TotalGems;

	if (TotalGems >= GemsNeeded)
	{
		ShopItemWidget->PurchaseItem(UserProgression, ShopItemWidget->GetBallIndex(), GemsNeeded);
		OnCloseButtonClicked();
	}

	else
	{
		OnCloseButtonClicked();
		ShopWidget->GemButtonClicked();
	}
}

void UInsufficientCoinsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	GameInstanceInterface = UFunctionsLibrary::GetGameInstanceInterface(this);

	CloseButton->OnClicked.AddDynamic(this, &UInsufficientCoinsWidget::OnCloseButtonClicked);
	GemsButton->OnClicked.AddDynamic(this, &UInsufficientCoinsWidget::OnGemsButtonClicked);
}

void UInsufficientCoinsWidget::OnCloseButtonClicked()
{
	SetVisibility(ESlateVisibility::Hidden);
	GemsNeeded = 0;
	ShopItemWidget = nullptr;
}