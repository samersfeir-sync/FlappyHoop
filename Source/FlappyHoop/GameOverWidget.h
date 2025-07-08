// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameOverWidget.generated.h"

class UButton;
class UTextBlock;
class UGameWidget;

UCLASS()
class FLAPPYHOOP_API UGameOverWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:

	void SetHighScoreText(int HighScore);
	void SetCurrentScoreText(int Score);
	void SetCoinsCollectedText(int Coins);
	void SetGemsCollectedText(int Gems);
	void SetGameWidgetRef(UGameWidget* NewGameWidget) { GameWidgetReference = NewGameWidget; }
	void InitializeWidget();

private:

	UPROPERTY(meta = (BindWidget))
	UTextBlock* CollectedCoinsText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* CollectedGemsText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* CurrentScoreText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* HighScoreText;

	UPROPERTY(meta = (BindWidget))
	UButton* QuitButton;

	UPROPERTY(meta = (BindWidget))
	UButton* RestartButton;

	UGameWidget* GameWidgetReference = nullptr;
};
