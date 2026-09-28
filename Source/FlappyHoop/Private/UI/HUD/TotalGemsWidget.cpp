// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/TotalGemsWidget.h"
#include "Components/TextBlock.h"

void UTotalGemsWidget::UpdateGemsText(int32 NewTotalGems)
{
	FString FormattedGems = FString::Printf(TEXT("%04d"), NewTotalGems);
	if (GemsText) GemsText->SetText(FText::FromString(FormattedGems));
}
