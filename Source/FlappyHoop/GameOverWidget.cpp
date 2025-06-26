// Fill out your copyright notice in the Description page of Project Settings.


#include "GameOverWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "GameWidget.h"
#include "FunctionsLibrary.h"

void UGameOverWidget::SetHighScoreText(int HighScore)
{
	UFunctionsLibrary::SetHighScoreText(HighScoreText, HighScore);
}

void UGameOverWidget::SetCurrentScoreText(int Score)
{
	CurrentScoreText->SetText(
		FText::FromString(FString::Printf(TEXT("Score: %d"), Score))
	);
}

void UGameOverWidget::SetCoinsCollectedText(int Coins)
{
	CollectedCoinsText->SetText(
		FText::FromString(FString::Printf(TEXT("+ %d"), Coins))
	);
}

void UGameOverWidget::InitializeWidget()
{
	if (GameWidgetReference)
	{
		QuitButton->OnClicked.AddDynamic(GameWidgetReference, &UGameWidget::QuitGame);
		RestartButton->OnClicked.AddDynamic(GameWidgetReference, &UGameWidget::ReturnToMainMenu);
	}
}
