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

	void ShowComboText();
	void HideComboText();
	void RefreshTotals();

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> PauseButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> ScoreText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> ComboText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UProgressBar> TimeProgressBar;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTotalCoinsWidget> TotalCoinsWidget;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTotalGemsWidget> TotalGemsWidget;

	/** Optional: shows the run's level, e.g. "Level 3" (format: LevelFormat). */
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> LevelText;

	/** Optional: plays when the level goes up (not at the start of a run). */
	UPROPERTY(meta = (BindWidgetAnimOptional), Transient)
	TObjectPtr<UWidgetAnimation> LevelUpAnimation;

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
