// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/Popups/QuitConfirmWidget.h"
#include "UI/HUD/GameWidget.h"
#include "Components/Button.h"

void UQuitConfirmWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	YesButton->OnClicked.AddUniqueDynamic(this, &UQuitConfirmWidget::OnYesClicked);
	NoButton->OnClicked.AddUniqueDynamic(this, &UQuitConfirmWidget::OnNoClicked);
}

void UQuitConfirmWidget::OnYesClicked()
{
	if (IsOpen() && GameWidget)
	{
		GameWidget->QuitGame();
	}
}

void UQuitConfirmWidget::OnNoClicked()
{
	Close();
}
