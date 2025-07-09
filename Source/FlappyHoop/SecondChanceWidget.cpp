// Fill out your copyright notice in the Description page of Project Settings.


#include "SecondChanceWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/CircularThrobber.h"
#include "GameModeInterface.h"
#include "FunctionsLibrary.h"
#include "Components/RichTextBlock.h"
#include "UserProgression.h"
#include "Ads/AGRewardItem.h"
#include "GameInstanceInterface.h"

void USecondChanceWidget::NativeConstruct()
{
	Super::NativeConstruct();

	GameModeInterface = UFunctionsLibrary::GetGameModeInterface(GetWorld());

	SkipAdButton->OnClicked.AddDynamic(this, &USecondChanceWidget::SkipAdButtonClicked);
	WatchAdButton->OnClicked.AddDynamic(this, &USecondChanceWidget::WatchAdButtonClicked);
	GemButton->OnClicked.AddDynamic(this, &USecondChanceWidget::GemButtonClicked);
}

void USecondChanceWidget::StartSkipTimer()
{
	GetWorld()->GetTimerManager().SetTimer(SkipTimerHandle, this, &USecondChanceWidget::DecreaseTimer, 1.0f, true);
}

void USecondChanceWidget::ResetWidget()
{
	GetWorld()->GetTimerManager().ClearTimer(SkipTimerHandle);
	SetVisibility(ESlateVisibility::Hidden);
	MaxSkipTime = 12;
	SkipCountdownText->SetText(FText::AsNumber(MaxSkipTime));
	LoadingAdThrobber->SetVisibility(ESlateVisibility::Hidden);
	SkipCountdownText->SetVisibility(ESlateVisibility::Visible);
	WatchAdButton->SetIsEnabled(true);
	SkipAdButton->SetIsEnabled(true);
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
		GemButton->SetIsEnabled(false);
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

void USecondChanceWidget::GemButtonClicked()
{
	IGameInstanceInterface* GameInstanceInterface = GameModeInterface->GetGameInstanceInterface();
	FUserProgression UserProgression = GameInstanceInterface->GetUserProgression();
	int32 GemsNeeded = GameModeInterface->GetGemsNeededForSecondChance();
	UserProgression.TotalGems -= GemsNeeded;
	GameInstanceInterface->SaveUserProgression(UserProgression);
	GameModeInterface->IncrementGemsSpentCount(GemsNeeded);

	GameModeInterface->IncrementRetryCount();
	GameModeInterface->IncrementGemsNeededForSecondChance();

	FRewardItem Reward;
	GameModeInterface->GrantSecondChance(Reward);
	ResetWidget();
}

void USecondChanceWidget::RewardAdFailed(int ErrorCode, FString ErrorMessage)
{
	if (GameModeInterface)
	{
		LoadingAdThrobber->SetVisibility(ESlateVisibility::Hidden);
		SkipCountdownText->SetVisibility(ESlateVisibility::Visible);
		WatchAdButton->SetIsEnabled(true);
		SkipAdButton->SetIsEnabled(true);
		GemButton->SetIsEnabled(true);
		ChangeMainText(ErrorMessage, true);
		MaxSkipTime = 12;
		SkipCountdownText->SetText(FText::AsNumber(MaxSkipTime));
		StartSkipTimer();
	}
}

void USecondChanceWidget::ChangeMainText(FString NewText, bool bErrorText)
{
	if (bErrorText)
	{
		NewText = FString::Printf(TEXT("<RedText>%s</>"), *NewText);
	}

	MainText->SetText(FText::FromString(NewText));
}
