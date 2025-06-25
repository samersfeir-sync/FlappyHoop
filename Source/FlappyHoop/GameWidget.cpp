// Fill out your copyright notice in the Description page of Project Settings.


#include "GameWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/HorizontalBox.h"
#include "Components/Border.h"
#include "GameModeInterface.h"
#include "FunctionsLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/Gameplaystatics.h"
#include "GameOverWidget.h"
#include "ComboProgressBarWidget.h"
#include "Components/Image.h"

void UGameWidget::NativeConstruct()
{
	Super::NativeConstruct();

	GameOverWidget->SetGameWidgetRef(this);
	GameOverWidget->InitializeWidget();

	GameModeInterface = UFunctionsLibrary::GetGameModeInterface(this);
	UpdateHighScoreUI();

	if (GameModeInterface)
	{
		PlayButton->OnClicked.AddDynamic(this, &UGameWidget::OnPlayClicked);
		GameModeInterface->OnViewportFetchedDelegate().AddUObject(this, &UGameWidget::EnablePlayButton);
		GameModeInterface->OnPointScoredDelegate().AddUObject(this, &UGameWidget::OnPointScored);
	}

	PauseButton->OnClicked.AddDynamic(this, &UGameWidget::PauseGame);
	ResumeButton->OnClicked.AddDynamic(this, &UGameWidget::ResumeGame);
	HomeButton->OnClicked.AddDynamic(this, &UGameWidget::ReturnToMainMenu);
	QuitButton->OnClicked.AddDynamic(this, &UGameWidget::QuitGame);

}

void UGameWidget::OnPlayClicked()
{
	GameModeInterface->OnGameStartedDelegate().Broadcast();
	ApplyWidgetState(EWidgetState::Playing);
	StartTimer();
}

void UGameWidget::QuitGame()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);

	UKismetSystemLibrary::QuitGame(World, PC, EQuitPreference::Quit, true);
}

void UGameWidget::PauseGame()
{
	UGameplayStatics::SetGamePaused(World, true);
	BlackBorder->SetVisibility(ESlateVisibility::Visible);
}

void UGameWidget::ResumeGame()
{
	UGameplayStatics::SetGamePaused(World, false);
	BlackBorder->SetVisibility(ESlateVisibility::Collapsed);
}

void UGameWidget::ReturnToMainMenu()
{
	GameModeInterface->ResetGame();
	ApplyWidgetState(EWidgetState::MainMenu);
	UpdateScoreUI(0);
	World->GetTimerManager().ClearTimer(GameTimer);
	TimeProgressBar->SetPercent(1.0f);
	EndComboTimer();
	UpdateHighScoreUI();
}

void UGameWidget::ShowGameOverWidget(bool bShow)
{
	ESlateVisibility GameOverWidgetVisibility = bShow ? ESlateVisibility::Visible : ESlateVisibility::Hidden;

	GameOverWidget->SetVisibility(GameOverWidgetVisibility);
}

void UGameWidget::EndComboTimer()
{
	ComboProgressBarWidget->EndTimer();
}

void UGameWidget::ShowComboText()
{
	int ScoreMultiplier = GameModeInterface->GetScoreMultiplier();

	if (ScoreMultiplier == 1)
		return;

	ComboText->SetText(
		FText::FromString(FString::Printf(TEXT("Combo! x%d"), ScoreMultiplier))
	);

	ComboText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	FWidgetAnimationDynamicEvent AnimationFinished;
	AnimationFinished.BindUFunction(this, FName("OnComboAnimationFinished"));
	BindToAnimationFinished(ComboTextAnimation, AnimationFinished);
	PlayAnimation(ComboTextAnimation);
}

void UGameWidget::OnComboAnimationFinished()
{
	ComboText->SetVisibility(ESlateVisibility::Hidden);
}

void UGameWidget::UpdateHighScoreUI()
{
	if (GameModeInterface)
	{
		int HighScore = GameModeInterface->GetHighScore();
		UFunctionsLibrary::SetHighScoreText(HighScoreText, HighScore);
	}
}

void UGameWidget::EnablePlayButton()
{
	PlayButton->SetIsEnabled(true);
}

void UGameWidget::ApplyWidgetState(EWidgetState NewState)
{
	switch (NewState)
	{
	case EWidgetState::MainMenu:
		PlayButton->SetVisibility(ESlateVisibility::Visible);
		QuitButton->SetVisibility(ESlateVisibility::Visible);
		ShopButton->SetVisibility(ESlateVisibility::Visible);
		HighScoreText->SetVisibility(ESlateVisibility::Visible);
		BouncyBucketsLogo->SetVisibility(ESlateVisibility::Visible);

		GameOverWidget->SetVisibility(ESlateVisibility::Hidden);
		PauseButton->SetVisibility(ESlateVisibility::Hidden);
		TimeProgressBar->SetVisibility(ESlateVisibility::Hidden);
		CoinsBox->SetVisibility(ESlateVisibility::Hidden);
		ScoreText->SetVisibility(ESlateVisibility::Hidden);
		BlackBorder->SetVisibility(ESlateVisibility::Hidden);
		break;

	case EWidgetState::Playing:
		PlayButton->SetVisibility(ESlateVisibility::Hidden);
		QuitButton->SetVisibility(ESlateVisibility::Hidden);
		ShopButton->SetVisibility(ESlateVisibility::Hidden);
		HighScoreText->SetVisibility(ESlateVisibility::Hidden);
		BouncyBucketsLogo->SetVisibility(ESlateVisibility::Hidden);

		PauseButton->SetVisibility(ESlateVisibility::Visible);
		TimeProgressBar->SetVisibility(ESlateVisibility::HitTestInvisible);
		CoinsBox->SetVisibility(ESlateVisibility::HitTestInvisible);
		ScoreText->SetVisibility(ESlateVisibility::HitTestInvisible);
		break;

	default:
		break;
	}
}

void UGameWidget::StartTimer()
{
	World->GetTimerManager().SetTimer(GameTimer, this, &UGameWidget::UpdateProgressBar, TickInterval, true);
}

void UGameWidget::UpdateProgressBar()
{
	float CurrentPercent = TimeProgressBar->GetPercent();
	ProgressStep = TickInterval / GameModeInterface->GetMaxGameTime();
	CurrentPercent -= ProgressStep;
	TimeProgressBar->SetPercent(FMath::Clamp(CurrentPercent, 0.0f, 1.0f));

	if (CurrentPercent <= 0.0f)
	{
		CurrentPercent = 0.0f;
		TimeProgressBar->SetPercent(CurrentPercent);
		World->GetTimerManager().PauseTimer(GameTimer);
		GameModeInterface->OnTimeEndedDelegate().Broadcast();
	}
}

void UGameWidget::OnPointScored()
{
	TimeProgressBar->SetPercent(1.0f);
	UpdateScoreUI(GameModeInterface->GetCurrentScore());
	World->GetTimerManager().UnPauseTimer(GameTimer);
	ShowComboText();
}

void UGameWidget::UpdateScoreUI(int NewScore)
{
	ScoreText->SetText(FText::AsNumber(NewScore));
}