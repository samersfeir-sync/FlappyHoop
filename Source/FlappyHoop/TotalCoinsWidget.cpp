// Fill out your copyright notice in the Description page of Project Settings.


#include "TotalCoinsWidget.h"
#include "Components/TextBlock.h"

void UTotalCoinsWidget::UpdateCoinsText(int NewTotalCoins)
{
	FString FormattedCoins = FString::Printf(TEXT("%04d"), NewTotalCoins);
	CoinsText->SetText(FText::FromString(FormattedCoins));
}
