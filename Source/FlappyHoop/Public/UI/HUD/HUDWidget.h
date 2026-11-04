// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Engine/TimerHandle.h"
#include "Core/GameStateEnum.h"
#include "HUDWidget.generated.h"

class UButton;
class UTextBlock;
class UProgressBar;
class UImage;
class UWidgetAnimation;
class UTotalCoinsWidget;
class UTotalGemsWidget;
class UGameWidget;
class ABaseGameMode;

/** In-run HUD: score, round timer bar, coin/gem totals, combo/swish text, pause button. */
UCLASS()
class FLAPPYHOOP_API UHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	void SetGameWidget(UGameWidget* NewGameWidget) { GameWidget = NewGameWidget; }

	/** Score 0, full timer bar, no combo text. */
	void ResetForNewRun();

protected:

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:

	UFUNCTION() void OnPauseClicked();
	UFUNCTION() void OnComboAnimationFinished();

	void OnRunPhaseChanged(EGameStateEnum OldPhase, EGameStateEnum NewPhase);
	void OnPointScored();
	void OnRoundTimeUpdated(float Fraction);
	void OnCoinCollected(bool bCoin);
	void OnBalanceChanged(int32 Coins, int32 Gems);
	void OnSecondChanceGranted();
	void OnLevelChanged(int32 Level);

	/** Bar percent, bar colour (green -> yellow -> red at LowTimeSeconds) and the seconds text. */
	void UpdateTimerDisplay();
	void UpdateLowTimeFlash(float DeltaSeconds);

	void ShowComboText();
	void HideComboText();
	void RefreshTotals();

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> PauseButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> ScoreText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> ComboText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UProgressBar> TimeProgressBar;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTotalCoinsWidget> TotalCoinsWidget;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTotalGemsWidget> TotalGemsWidget;

	/** Optional: seconds remaining, placed over the centre of TimeProgressBar (format: TimeTextFormat). */
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> TimeText;

	/** Optional: full-screen red Image (anchors stretched to fill) that flashes when time is low. Turn it on/off with bLowTimeScreenFlash on the game mode. */
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UImage> LowTimeFlashImage;

	/** {0} = whole seconds remaining (rounded up). E.g. "{0}" or "{0}s". */
	UPROPERTY(EditDefaultsOnly, Category = "Timer")
	FText TimeTextFormat = NSLOCTEXT("HUD", "TimeTextFormat", "{0}");

	/** Bar colour with a full round. Set the bar's Fill Image tint to white so these show as-is. */
	UPROPERTY(EditDefaultsOnly, Category = "Timer")
	FLinearColor TimerFullColor = FLinearColor(0.1f, 0.85f, 0.2f);

	/** Bar colour halfway between a full round and the low-time point. */
	UPROPERTY(EditDefaultsOnly, Category = "Timer")
	FLinearColor TimerMidColor = FLinearColor(1.0f, 0.8f, 0.0f);

	/** Bar colour at and below the game mode's LowTimeSeconds. */
	UPROPERTY(EditDefaultsOnly, Category = "Timer")
	FLinearColor TimerLowColor = FLinearColor(0.95f, 0.05f, 0.05f);

	/** Peak opacity of LowTimeFlashImage ("lightly" = low). */
	UPROPERTY(EditDefaultsOnly, Category = "Timer|Flash", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FlashMaxOpacity = 0.25f;

	/** Flashes per second. */
	UPROPERTY(EditDefaultsOnly, Category = "Timer|Flash", meta = (ClampMin = "0.1"))
	float FlashesPerSecond = 2.0f;

	float FlashTime = 0.0f;

	/** Optional: shows the run's level, e.g. "Level 3" (format: LevelFormat). */
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> LevelText;

	/** Optional: plays when the level goes up (not at the start of a run). */
	UPROPERTY(meta = (BindWidgetAnimOptional), Transient)
	TObjectPtr<UWidgetAnimation> LevelUpAnimation;

	/** Optional: "Easy" / "Hard" for the current run. */
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> DifficultyText;

	UPROPERTY(EditDefaultsOnly, Category = "Difficulty")
	FText EasyLabel = NSLOCTEXT("HUD", "Easy", "Easy");

	UPROPERTY(EditDefaultsOnly, Category = "Difficulty")
	FText HardLabel = NSLOCTEXT("HUD", "Hard", "Hard");

	void UpdateDifficultyText();

	/** {0} = level number. */
	UPROPERTY(EditDefaultsOnly, Category = "Level")
	FText LevelFormat = NSLOCTEXT("HUD", "LevelFormat", "Level {0}");

	/** Optional: plays on the combo text. Without it the text simply shows for ComboTextSeconds. */
	UPROPERTY(meta = (BindWidgetAnimOptional), Transient)
	TObjectPtr<UWidgetAnimation> ComboTextAnimation;

	/** How long the combo text stays up when there's no ComboTextAnimation. */
	UPROPERTY(EditDefaultsOnly, Category = "Combo", meta = (ClampMin = "0.1"))
	float ComboTextSeconds = 1.0f;

	FTimerHandle ComboTextTimer;

	UPROPERTY(Transient)
	TObjectPtr<UGameWidget> GameWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<ABaseGameMode> GameMode = nullptr;
};
