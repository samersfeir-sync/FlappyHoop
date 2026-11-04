// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Popups/GameOverWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "UI/HUD/GameWidget.h"
#include "Core/FunctionsLibrary.h"
#include "Framework/BaseGameMode.h"
#include "Systems/LeaderboardSubsystem.h"

void UGameOverWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	HomeButton->OnClicked.AddUniqueDynamic(this, &UGameOverWidget::OnHomeClicked);
	RestartButton->OnClicked.AddUniqueDynamic(this, &UGameOverWidget::OnRestartClicked);

	// Always shown (also in the editor/desktop, where pressing it does nothing).
	if (LeaderboardButton)
	{
		LeaderboardButton->OnClicked.AddUniqueDynamic(this, &UGameOverWidget::OnLeaderboardClicked);
		LeaderboardButton->SetVisibility(ESlateVisibility::Visible);
	}
	if (LeaderboardModeText)
	{
		LeaderboardModeText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
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

	if (LeaderboardModeText)
	{
		LeaderboardModeText->SetText(GameMode->GetDifficulty() == EDifficultyMode::Hard ? LeaderboardHardLabel : LeaderboardEasyLabel);
	}
}

void UGameOverWidget::OnHomeClicked()
{
	if (IsOpen() && GameWidget)
	{
		GameWidget->ReturnToMainMenu();
	}
}

void UGameOverWidget::OnRestartClicked()
{
	if (IsOpen() && GameWidget)
	{
		GameWidget->RestartGame(); // straight into a new run, same difficulty
	}
}

void UGameOverWidget::OnLeaderboardClicked()
{
	if (!IsOpen())
	{
		return;
	}

	// The difficulty can't change until the player is back in the menu, so this is the mode just played.
	const ABaseGameMode* GameMode = ABaseGameMode::Get(this);
	if (ULeaderboardSubsystem* Leaderboards = ULeaderboardSubsystem::Get(this))
	{
		Leaderboards->ShowLeaderboard(GameMode ? GameMode->GetDifficulty() : EDifficultyMode::Easy); // signs in first if needed
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
