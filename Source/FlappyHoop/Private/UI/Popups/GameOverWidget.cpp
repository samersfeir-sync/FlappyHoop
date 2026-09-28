// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Popups/GameOverWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "UI/HUD/GameWidget.h"
#include "Core/FunctionsLibrary.h"
#include "Framework/BaseGameMode.h"

void UGameOverWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	QuitButton->OnClicked.AddUniqueDynamic(this, &UGameOverWidget::OnQuitClicked);
	RestartButton->OnClicked.AddUniqueDynamic(this, &UGameOverWidget::OnRestartClicked);
}

void UGameOverWidget::OnPopupOpening()
{
	const ABaseGameMode* GameMode = ABaseGameMode::Get(this);
	if (!GameMode)
	{
		return;
	}

	SetHighScoreText(GameMode->GetHighScore());
	SetCurrentScoreText(GameMode->GetCurrentScore());
	SetCoinsCollectedText(GameMode->GetCollectedCoins());
	SetGemsCollectedText(GameMode->GetCollectedGems() - GameMode->GetGemsSpent());
}

void UGameOverWidget::OnQuitClicked()
{
	if (IsOpen() && GameWidget)
	{
		GameWidget->RequestQuit(); // asks "Quit the game?" first
	}
}

void UGameOverWidget::OnRestartClicked()
{
	if (IsOpen() && GameWidget)
	{
		GameWidget->ReturnToMainMenu();
	}
}

void UGameOverWidget::SetHighScoreText(int32 HighScore)
{
	UFunctionsLibrary::SetHighScoreText(HighScoreText, HighScore);
}

void UGameOverWidget::SetCurrentScoreText(int32 Score)
{
	CurrentScoreText->SetText(FText::FromString(FString::Printf(TEXT("Score: %d"), Score)));
}

void UGameOverWidget::SetCoinsCollectedText(int32 Coins)
{
	CollectedCoinsText->SetText(FText::FromString(FString::Printf(TEXT("+%d"), Coins)));
}

void UGameOverWidget::SetGemsCollectedText(int32 Gems)
{
	const TCHAR* Prefix = (Gems >= 0) ? TEXT("+") : TEXT("-");
	CollectedGemsText->SetText(FText::FromString(FString::Printf(TEXT("%s%d"), Prefix, FMath::Abs(Gems))));
}
