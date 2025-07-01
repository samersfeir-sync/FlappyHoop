// Fill out your copyright notice in the Description page of Project Settings.


#include "SecondChanceWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/CircularThrobber.h"
#include "GameModeInterface.h"
#include "FunctionsLibrary.h"

void USecondChanceWidget::NativeConstruct()
{
	Super::NativeConstruct();

	GameModeInterface = UFunctionsLibrary::GetGameModeInterface(GetWorld());

	if (GameModeInterface && GameModeInterface->GetRewardedAdInterface())
	{
		FailedToLoadDelegate.BindDynamic(this, &USecondChanceWidget::FailedToLoadSecondChanceAd);
		GameModeInterface->GetRewardedAdInterface()->BindEventToOnAdFailedToLoad(FailedToLoadDelegate);
		FailedToShowDelegate.BindDynamic(this, &USecondChanceWidget::FailedToLoadSecondChanceAd);
		GameModeInterface->GetRewardedAdInterface()->BindEventToOnAdFailedToShow(FailedToShowDelegate);
	}

	SkipAdButton->OnClicked.AddDynamic(this, &USecondChanceWidget::SkipAdButtonClicked);
	WatchAdButton->OnClicked.AddDynamic(this, &USecondChanceWidget::WatchAdButtonClicked);
}

void USecondChanceWidget::StartSkipTimer()
{
	GetWorld()->GetTimerManager().SetTimer(SkipTimerHandle, this, &USecondChanceWidget::DecreaseTimer, 1.0f, true);
}

void USecondChanceWidget::ResetWidget()
{
	GetWorld()->GetTimerManager().ClearTimer(SkipTimerHandle);
	SetVisibility(ESlateVisibility::Hidden);
	ChangeMainText("Don't give up yet! Watch an ad for a second shot!", FLinearColor::White);
	MaxSkipTime = 12;
	SkipCountdownText->SetText(FText::AsNumber(MaxSkipTime));
}

void USecondChanceWidget::DecreaseTimer()
{
	if (MaxSkipTime > 0)
	{
		MaxSkipTime--;
		SkipCountdownText->SetText(FText::AsNumber(MaxSkipTime));

		if (MaxSkipTime <= 0)
		{
			SkipAdButtonClicked();
		}
	}
}

void USecondChanceWidget::WatchAdButtonClicked()
{
	if (GameModeInterface)
	{
		GetWorld()->GetTimerManager().ClearTimer(SkipTimerHandle);
		LoadingAdThrobber->SetVisibility(ESlateVisibility::Visible);
		SkipCountdownText->SetVisibility(ESlateVisibility::Hidden);
		WatchAdButton->SetIsEnabled(false);
		SkipAdButton->SetIsEnabled(false);
		GameModeInterface->LoadRewardedAd();
	}
}

void USecondChanceWidget::SkipAdButtonClicked()
{
	ResetWidget();

	if (GameModeInterface)
	{
		GameModeInterface->EndGame();
	}
}

void USecondChanceWidget::FailedToLoadSecondChanceAd(int ErrorCode, FString ErrorMessage)
{
	if (GameModeInterface)
	{
		LoadingAdThrobber->SetVisibility(ESlateVisibility::Hidden);
		SkipCountdownText->SetVisibility(ESlateVisibility::Visible);
		WatchAdButton->SetIsEnabled(true);
		SkipAdButton->SetIsEnabled(true);
		ChangeMainText(ErrorMessage, FLinearColor::Red);
		SkipCountdownText->SetText(FText::AsNumber(12));
		StartSkipTimer();
	}
}

void USecondChanceWidget::ChangeMainText(FString NewText, FLinearColor NewColor)
{
	MainText->SetText(FText::FromString(NewText));
	MainText->SetColorAndOpacity(NewColor);
}
