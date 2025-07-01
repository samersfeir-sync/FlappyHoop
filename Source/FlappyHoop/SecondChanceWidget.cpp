// Fill out your copyright notice in the Description page of Project Settings.


#include "SecondChanceWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/CircularThrobber.h"
#include "GameModeInterface.h"
#include "FunctionsLibrary.h"
#include "Interface/AGRewardedAdInterface.h"

void USecondChanceWidget::NativeConstruct()
{
	Super::NativeConstruct();

	GameModeInterface = UFunctionsLibrary::GetGameModeInterface(GetWorld());

	if (GameModeInterface && GameModeInterface->GetRewardedAdInterface())
	{
		FOnRewardedAdFailedToLoadDelegate FailedToLoadDelegate;
		FailedToLoadDelegate.BindDynamic(this, &USecondChanceWidget::FailedToLoadSecondChanceAd);
		GameModeInterface->GetRewardedAdInterface()->BindEventToOnAdFailedToLoad(FailedToLoadDelegate);
	}

	StartSkipTimer();
	SkipAdButton->OnClicked.AddDynamic(this, &USecondChanceWidget::SkipAdButtonClicked);
	WatchAdButton->OnClicked.AddDynamic(this, &USecondChanceWidget::WatchAdButtonClicked);
}

void USecondChanceWidget::StartSkipTimer()
{
	GetWorld()->GetTimerManager().SetTimer(SkipTimerHandle, this, &USecondChanceWidget::DecreaseTimer, 1.0f, true);
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
		LoadingAdThrobber->SetVisibility(ESlateVisibility::Visible);
		SkipCountdownText->SetVisibility(ESlateVisibility::Hidden);
		WatchAdButton->SetIsEnabled(false);
		SkipAdButton->SetIsEnabled(false);
		GameModeInterface->LoadRewardedAd();
	}
}

void USecondChanceWidget::SkipAdButtonClicked()
{
	RemoveFromParent();

	if (GameModeInterface)
	{
		GameModeInterface->DereferenceSecondChanceWidget();
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
	}
}

void USecondChanceWidget::ChangeMainText(FString NewText, FLinearColor NewColor)
{
	MainText->SetText(FText::FromString(NewText));
	MainText->SetColorAndOpacity(NewColor);
}
