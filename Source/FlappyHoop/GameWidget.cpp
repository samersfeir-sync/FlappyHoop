// Fill out your copyright notice in the Description page of Project Settings.


#include "GameWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/HorizontalBox.h"
#include "GameModeInterface.h"
#include "FunctionsLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/Gameplaystatics.h"

void UGameWidget::NativeConstruct()
{
	Super::NativeConstruct();

	GameModeInterface = UFunctionsLibrary::GetGameModeInterface(this);

	if (GameModeInterface)
	{
		PlayButton->OnClicked.AddDynamic(this, &UGameWidget::OnPlayClicked);
		GameModeInterface->OnFOnViewportFetchedDelegate().AddUObject(this, &UGameWidget::EnablePlayButton);
	}

	QuitButton->OnClicked.AddDynamic(this, &UGameWidget::QuitGame);
}

void UGameWidget::OnPlayClicked()
{
	GameModeInterface->OnGameStartedDelegate().Broadcast();

	PlayButton->SetVisibility(ESlateVisibility::Hidden);
	QuitButton->SetVisibility(ESlateVisibility::Hidden);
	ShopButton->SetVisibility(ESlateVisibility::Hidden);
	HighScoreText->SetVisibility(ESlateVisibility::Hidden);

	PauseButton->SetVisibility(ESlateVisibility::Visible);
	TimeProgressBar->SetVisibility(ESlateVisibility::HitTestInvisible);
	CoinsBox->SetVisibility(ESlateVisibility::HitTestInvisible);
	ScoreText->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UGameWidget::QuitGame()
{
	UWorld* World = GetWorld();
	if (!World) return;

	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	if (!PC) return;

	UKismetSystemLibrary::QuitGame(World, PC, EQuitPreference::Quit, true);
}

void UGameWidget::EnablePlayButton()
{
	PlayButton->SetIsEnabled(true);
}
