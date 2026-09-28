// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/TotalCoinsWidget.h"
#include "Components/TextBlock.h"

void UTotalCoinsWidget::UpdateCoinsText(int32 NewTotalCoins)
{
	FString FormattedCoins = FString::Printf(TEXT("%04d"), NewTotalCoins);
	if (CoinsText) CoinsText->SetText(FText::FromString(FormattedCoins));
}
