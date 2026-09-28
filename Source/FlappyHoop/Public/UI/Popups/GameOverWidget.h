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

	/** The UI root, which handles Quit and Restart. */
	void SetGameWidget(UGameWidget* NewGameWidget) { GameWidget = NewGameWidget; }

protected:

	virtual void NativeOnInitialized() override;
	virtual void OnPopupOpening() override;

private:

	UFUNCTION()
	void OnQuitClicked();

	UFUNCTION()
	void OnRestartClicked();

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
	TObjectPtr<UButton> QuitButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> RestartButton;

	UPROPERTY(Transient)
	TObjectPtr<UGameWidget> GameWidget = nullptr;
};
