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
#include "Components/Image.h"
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
		GameMode->OnLevelChanged.AddUObject(this, &UHUDWidget::OnLevelChanged);
	}

	if (LevelText)
	{
		LevelText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (DifficultyText)
	{
		DifficultyText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (TimeText)
	{
		TimeText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (LowTimeFlashImage)
	{
		// Never blocks taps; starts invisible.
		LowTimeFlashImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		LowTimeFlashImage->SetRenderOpacity(0.0f);
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
		GameMode->OnLevelChanged.RemoveAll(this);
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

void UHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	UpdateLowTimeFlash(InDeltaTime);
}

void UHUDWidget::ResetForNewRun()
{
	ScoreText->SetText(FText::AsNumber(0));
	TimeProgressBar->SetPercent(1.0f);
	UpdateTimerDisplay();
	FlashTime = 0.0f;
	if (LowTimeFlashImage)
	{
		LowTimeFlashImage->SetRenderOpacity(0.0f);
	}
	HideComboText();
	RefreshTotals();

	if (LevelText)
	{
		LevelText->SetText(FText::Format(LevelFormat, FText::AsNumber(1)));
	}

	UpdateDifficultyText();
}

void UHUDWidget::UpdateDifficultyText()
{
	if (DifficultyText && GameMode)
	{
		DifficultyText->SetText(GameMode->GetDifficulty() == EDifficultyMode::Hard ? HardLabel : EasyLabel);
	}
}

void UHUDWidget::OnLevelChanged(int32 Level)
{
	if (LevelText)
	{
		LevelText->SetText(FText::Format(LevelFormat, FText::AsNumber(Level)));
	}

	if (Level > 1 && LevelUpAnimation)
	{
		PlayAnimation(LevelUpAnimation);
	}
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

	// The mode is picked in the menu, so refresh it when a run starts.
	if (NewPhase == EGameStateEnum::Playing)
	{
		UpdateDifficultyText();
	}
}

void UHUDWidget::OnPointScored()
{
	if (!GameMode)
	{
		return;
	}

	// Time back per basket depends on the difficulty, so the bar isn't always full again.
	UpdateTimerDisplay();
	ScoreText->SetText(FText::AsNumber(GameMode->GetCurrentScore()));
	ShowComboText();
}

void UHUDWidget::OnRoundTimeUpdated(float Fraction)
{
	UpdateTimerDisplay();
}

void UHUDWidget::UpdateTimerDisplay()
{
	if (!GameMode)
	{
		return;
	}

	const float Remaining = GameMode->GetRoundTimeRemaining();
	const float RoundLength = GameMode->GetCurrentRoundLength();
	const float LowTime = GameMode->LowTimeSeconds;

	TimeProgressBar->SetPercent(GameMode->GetRoundTimeFraction());

	// 1 with a full round, 0 at LowTimeSeconds and below. If the whole round is <= LowTimeSeconds, it's always red.
	const float Range = RoundLength - LowTime;
	const float Alpha = Range > KINDA_SMALL_NUMBER ? FMath::Clamp((Remaining - LowTime) / Range, 0.0f, 1.0f) : 0.0f;

	const FLinearColor BarColor = Alpha >= 0.5f
		? FLinearColor::LerpUsingHSV(TimerMidColor, TimerFullColor, (Alpha - 0.5f) * 2.0f)
		: FLinearColor::LerpUsingHSV(TimerLowColor, TimerMidColor, Alpha * 2.0f);
	TimeProgressBar->SetFillColorAndOpacity(BarColor);

	if (TimeText)
	{
		const int32 Seconds = FMath::Max(0, FMath::CeilToInt(Remaining - KINDA_SMALL_NUMBER));
		TimeText->SetText(FText::Format(TimeTextFormat, FText::AsNumber(Seconds)));
	}
}

void UHUDWidget::UpdateLowTimeFlash(float DeltaSeconds)
{
	if (!LowTimeFlashImage)
	{
		return;
	}

	const UWorld* World = GetWorld();
	const bool bFlash = GameMode
		&& GameMode->bLowTimeScreenFlash
		&& GameMode->GetCurrentGameState() == EGameStateEnum::Playing
		&& !GameMode->GetTimeEndedBool()
		&& !(World && World->IsPaused())
		&& GameMode->GetRoundTimeRemaining() <= GameMode->LowTimeSeconds;

	if (!bFlash)
	{
		FlashTime = 0.0f;
		LowTimeFlashImage->SetRenderOpacity(0.0f);
		return;
	}

	// Smooth on/off pulse: starts at 0, peaks at FlashMaxOpacity, FlashesPerSecond times a second.
	FlashTime += DeltaSeconds;
	const float Pulse = 0.5f - 0.5f * FMath::Cos(FlashTime * FlashesPerSecond * 2.0f * PI);
	LowTimeFlashImage->SetRenderOpacity(Pulse * FlashMaxOpacity);
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
	UpdateTimerDisplay();
}

void UHUDWidget::ShowComboText()
{
	// A basket that didn't build the combo (swish-only mode) shows no combo text.
	const int32 ScoreMultiplier = GameMode->DidLastBasketBuildCombo() ? GameMode->GetScoreMultiplier() : 1;
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
