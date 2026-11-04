// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/Common/PopupWidget.h"
#include "GameOverWidget.generated.h"

class UButton;
class UTextBlock;
class UGameWidget;

/** End-of-run results. Fills itself from the game mode when it opens. */
UCLASS()
class FLAPPYHOOP_API UGameOverWidget : public UPopupWidget
{
	GENERATED_BODY()

public:

	/** The UI root, which handles Home and Restart. */
	void SetGameWidget(UGameWidget* NewGameWidget) { GameWidget = NewGameWidget; }

protected:

	virtual void NativeOnInitialized() override;
	virtual void OnPopupOpening() override;

private:

	UFUNCTION()
	void OnHomeClicked();

	UFUNCTION()
	void OnRestartClicked();

	UFUNCTION()
	void OnLeaderboardClicked();

	void SetHighScoreText(int32 HighScore);
	void SetCurrentScoreText(int32 Score);
	void SetCoinsCollectedText(int32 Coins);
	void SetGemsCollectedText(int32 Gems);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CollectedCoinsText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CollectedGemsText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CurrentScoreText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> HighScoreText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> HomeButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> RestartButton;

	/** Optional: opens the leaderboard of the difficulty just played. Hidden where there are no leaderboards (editor, desktop). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> LeaderboardButton;

	/** Optional: "EASY" / "HARD" badge for LeaderboardButton. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LeaderboardModeText;

	UPROPERTY(EditDefaultsOnly, Category = "Leaderboard")
	FText LeaderboardEasyLabel = NSLOCTEXT("GameOver", "LeaderboardEasy", "EASY");

	UPROPERTY(EditDefaultsOnly, Category = "Leaderboard")
	FText LeaderboardHardLabel = NSLOCTEXT("GameOver", "LeaderboardHard", "HARD");

	UPROPERTY(Transient)
	TObjectPtr<UGameWidget> GameWidget = nullptr;
};
