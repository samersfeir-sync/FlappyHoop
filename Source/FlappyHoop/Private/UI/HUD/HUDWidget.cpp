// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/HUD/HUDWidget.h"
#include "UI/HUD/GameWidget.h"
#include "UI/HUD/TotalCoinsWidget.h"
#include "UI/HUD/TotalGemsWidget.h"
#include "Framework/BaseGameMode.h"
#include "Systems/EconomySubsystem.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Animation/WidgetAnimation.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	GameMode = ABaseGameMode::Get(this);
	if (GameMode)
	{
		GameMode->OnRunPhaseChanged.AddUObject(this, &UHUDWidget::OnRunPhaseChanged);
		GameMode->OnPointScored.AddUObject(this, &UHUDWidget::OnPointScored);
		GameMode->OnRoundTimeUpdated.AddUObject(this, &UHUDWidget::OnRoundTimeUpdated);
		GameMode->OnCoinCollected.AddUObject(this, &UHUDWidget::OnCoinCollected);
		GameMode->OnSecondChanceGranted.AddUObject(this, &UHUDWidget::OnSecondChanceGranted);
	}

	if (UEconomySubsystem* Economy = UEconomySubsystem::Get(this))
	{
		Economy->OnBalanceChanged.AddUObject(this, &UHUDWidget::OnBalanceChanged);
	}

	PauseButton->OnClicked.AddUniqueDynamic(this, &UHUDWidget::OnPauseClicked);

	// The old GameWidget kept these Hidden in the designer and showed them from code; the HUD as a whole is now
	// shown/hidden by the UI root, so make sure the pieces themselves are always on.
	PauseButton->SetVisibility(ESlateVisibility::Visible);
	ScoreText->SetVisibility(ESlateVisibility::HitTestInvisible);
	TimeProgressBar->SetVisibility(ESlateVisibility::HitTestInvisible);
	TotalCoinsWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	TotalGemsWidget->SetVisibility(ESlateVisibility::HitTestInvisible);

	if (ComboTextAnimation)
	{
		FWidgetAnimationDynamicEvent AnimationFinished;
		AnimationFinished.BindDynamic(this, &UHUDWidget::OnComboAnimationFinished);
		BindToAnimationFinished(ComboTextAnimation, AnimationFinished);
	}

	ResetForNewRun();
}

void UHUDWidget::NativeDestruct()
{
	if (GameMode)
	{
		GameMode->OnRunPhaseChanged.RemoveAll(this);
		GameMode->OnPointScored.RemoveAll(this);
		GameMode->OnRoundTimeUpdated.RemoveAll(this);
		GameMode->OnCoinCollected.RemoveAll(this);
		GameMode->OnSecondChanceGranted.RemoveAll(this);
	}

	if (UEconomySubsystem* Economy = UEconomySubsystem::Get(this))
	{
		Economy->OnBalanceChanged.RemoveAll(this);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ComboTextTimer);
	}

	Super::NativeDestruct();
}

void UHUDWidget::ResetForNewRun()
{
	ScoreText->SetText(FText::AsNumber(0));
	TimeProgressBar->SetPercent(1.0f);
	HideComboText();
	RefreshTotals();
}

void UHUDWidget::OnPauseClicked()
{
	if (GameWidget)
	{
		GameWidget->PauseGame();
	}
}

void UHUDWidget::OnRunPhaseChanged(EGameStateEnum OldPhase, EGameStateEnum NewPhase)
{
	// Run coins are banked at game over; stop adding them on top.
	RefreshTotals();
}

void UHUDWidget::OnPointScored()
{
	if (!GameMode)
	{
		return;
	}

	TimeProgressBar->SetPercent(1.0f);
	ScoreText->SetText(FText::AsNumber(GameMode->GetCurrentScore()));
	ShowComboText();
}

void UHUDWidget::OnRoundTimeUpdated(float Fraction)
{
	TimeProgressBar->SetPercent(Fraction);
}

void UHUDWidget::OnCoinCollected(bool bCoin)
{
	RefreshTotals();
}

void UHUDWidget::OnBalanceChanged(int32 Coins, int32 Gems)
{
	RefreshTotals();
}

void UHUDWidget::OnSecondChanceGranted()
{
	RefreshTotals();
	TimeProgressBar->SetPercent(1.0f);
}

void UHUDWidget::ShowComboText()
{
	const int32 ScoreMultiplier = GameMode->GetScoreMultiplier();
	const bool bSwish = GameMode->WasLastBasketSwish();

	FString Text;
	if (bSwish && ScoreMultiplier > 1)
	{
		Text = FString::Printf(TEXT("Swish!\nCombo! x%d"), ScoreMultiplier); // two lines: swish on top, combo under it
	}
	else if (bSwish)
	{
		Text = TEXT("Swish!");
	}
	else if (ScoreMultiplier > 1)
	{
		Text = FString::Printf(TEXT("Combo! x%d"), ScoreMultiplier);
	}
	else
	{
		return;
	}

	ComboText->SetText(FText::FromString(Text));
	ComboText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (ComboTextAnimation)
	{
		PlayAnimation(ComboTextAnimation);
	}
	else if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(ComboTextTimer, this, &UHUDWidget::HideComboText, ComboTextSeconds, false);
	}
}

void UHUDWidget::OnComboAnimationFinished()
{
	HideComboText();
}

void UHUDWidget::HideComboText()
{
	ComboText->SetVisibility(ESlateVisibility::Hidden);
}

void UHUDWidget::RefreshTotals()
{
	const UEconomySubsystem* Economy = UEconomySubsystem::Get(this);
	if (!Economy)
	{
		return;
	}

	// Coins picked up this run are only banked at game over, so show banked + pending during a run.
	const EGameStateEnum Phase = GameMode ? GameMode->GetCurrentGameState() : EGameStateEnum::MainMenu;
	const bool bInRun = Phase != EGameStateEnum::MainMenu && Phase != EGameStateEnum::GameOver;
	const int32 PendingCoins = (bInRun && GameMode) ? GameMode->GetCollectedCoins() : 0;

	TotalCoinsWidget->UpdateCoinsText(Economy->GetCoins() + PendingCoins);
	TotalGemsWidget->UpdateGemsText(Economy->GetGems());
}
