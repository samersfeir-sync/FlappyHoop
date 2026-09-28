// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/Popups/PauseWidget.h"
#include "UI/HUD/GameWidget.h"
#include "Components/Button.h"
#include "Components/Widget.h"
#include "Core/FlappyHoop.h"

void UPauseWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	ResumeButton->OnClicked.AddUniqueDynamic(this, &UPauseWidget::OnResumeClicked);
	HomeButton->OnClicked.AddUniqueDynamic(this, &UPauseWidget::OnHomeClicked);
	SettingsButton->OnClicked.AddUniqueDynamic(this, &UPauseWidget::OnSettingsClicked);

	// The popup itself is shown by scaling; its contents must simply be on.
	if (BlackBorder)
	{
		BlackBorder->SetVisibility(ESlateVisibility::Visible);
	}
	ResumeButton->SetVisibility(ESlateVisibility::Visible);
	HomeButton->SetVisibility(ESlateVisibility::Visible);
	SettingsButton->SetVisibility(ESlateVisibility::Visible);
}

void UPauseWidget::OnResumeClicked()
{
	if (IsOpen() && GameWidget)
	{
		GameWidget->ResumeGame();
	}
}

void UPauseWidget::OnHomeClicked()
{
	if (IsOpen() && GameWidget)
	{
		GameWidget->ReturnToMainMenu();
	}
}

void UPauseWidget::OnSettingsClicked()
{
	if (IsOpen() && GameWidget)
	{
		GameWidget->OpenSettings();
	}
}
