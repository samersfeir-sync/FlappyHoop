// Fill out your copyright notice in the Description page of Project Settings.


#include "GameWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/Border.h"
#include "GameModeInterface.h"
#include "FunctionsLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/Gameplaystatics.h"
#include "GameOverWidget.h"
#include "ComboProgressBarWidget.h"
#include "Components/Image.h"
#include "TotalCoinsWidget.h"
#include "ShopWidget.h"
#include "GameInstanceInterface.h"
#include "SecondChanceWidget.h"
#include "SettingsWidget.h"
#include "TotalGemsWidget.h"

void UGameWidget::NativeConstruct()
{
	Super::NativeConstruct();

	GameOverWidget->SetGameWidgetRef(this);
	GameOverWidget->InitializeWidget();

	GameModeInterface = UFunctionsLibrary::GetGameModeInterface(this);
	UpdateHighScoreUI();
	UpdateCollectiblesUI(true); //coins
	UpdateCollectiblesUI(false); //gems

	if (GameModeInterface)
	{
		PlayButton->OnClicked.AddDynamic(this, &UGameWidget::OnPlayClicked);
		//GameModeInterface->OnViewportFetchedDelegate().AddUObject(this, &UGameWidget::EnablePlayButton);
		GameModeInterface->OnPointScoredDelegate().AddUObject(this, &UGameWidget::OnPointScored);
		GameModeInterface->OnCoinCollectedDelegate().AddUObject(this, &UGameWidget::UpdateCollectiblesUI);
		GameModeInterface->OnSecondChanceGrantedDelegate().AddUObject(this, &UGameWidget::PauseGameAfterRewardAD);
		GameInstanceInterface = GameModeInterface->GetGameInstanceInterface();
	}

	PauseButton->OnClicked.AddDynamic(this, &UGameWidget::PauseGame);
	ResumeButton->OnClicked.AddDynamic(this, &UGameWidget::ResumeGame);
	HomeButton->OnClicked.AddDynamic(this, &UGameWidget::ReturnToMainMenu);
	QuitButton->OnClicked.AddDynamic(this, &UGameWidget::QuitGame);
	ShopButton->OnClicked.AddDynamic(this, &UGameWidget::ShowShopWidget);
	SettingsButton->OnClicked.AddDynamic(this, &UGameWidget::SettingsButtonClicked);
	PauseSettingsButton->OnClicked.AddDynamic(this, &UGameWidget::SettingsButtonClicked);
	NoAdsButton->OnClicked.AddDynamic(this, &UGameWidget::NoAdsButtonClicked);

	HideNoAdsButton();

	ShopWidget->AdsRemovedDelegate.BindUObject(this, &UGameWidget::HideNoAdsButton);
}

void UGameWidget::OnPlayClicked()
{
	GameModeInterface->OnGameStartedDelegate().Broadcast();
	CurrentWidgetState = EWidgetState::Playing;
	ApplyWidgetState(CurrentWidgetState);
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

void UGameWidget::ShowSecondChanceWidget(bool bShow)
{
	ESlateVisibility SecondChanceWidgetVisibility = bShow ? ESlateVisibility::Visible : ESlateVisibility::Hidden;

	SecondChanceWidget->SetVisibility(SecondChanceWidgetVisibility);

	if (!bShow)
	{
		SecondChanceWidget->ResetWidget();
		return;
	}

	bool bCanWatchAd = GameModeInterface->GetCanWatchAd();
	SecondChanceWidget->WatchAdButton->SetIsEnabled(bCanWatchAd);
	
	int32 GemsNeeded = GameModeInterface->GetGemsNeededForSecondChance();
	int32 TotalGems = GameModeInterface->GetTotalGems();
	bool bCanUseGems = TotalGems >= GemsNeeded;
	SecondChanceWidget->GemButton->SetIsEnabled(bCanUseGems);
	
	FString GemText = FString::Printf(TEXT("%d gem%s"), GemsNeeded, GemsNeeded == 1 ? TEXT("") : TEXT("s"));
	FString UseGemsOnly = FString::Printf(TEXT("Don't give up yet! Use <CyanText>%s</> for a second shot!"), *GemText);
	FString BothOptions = FString::Printf(TEXT("Don't give up yet! Watch an ad or use <CyanText>%s</> for a second shot!"), *GemText);
	FString WatchAdOnly = FString::Printf(TEXT("Don't give up yet! Watch an ad for a second shot!"));

	SecondChanceWidget->ChangeMainText(
		bCanWatchAd && TotalGems >= GemsNeeded ? BothOptions :
		bCanWatchAd ? WatchAdOnly :
		UseGemsOnly,
		false
	);

	SecondChanceWidget->TotalGemsWidget->UpdateGemsText(TotalGems);
	SecondChanceWidget->StartSkipTimer();
}

void UGameWidget::ResumeGame()
{
	UGameplayStatics::SetGamePaused(World, false);
	BlackBorder->SetVisibility(ESlateVisibility::Collapsed);
}

void UGameWidget::ReturnToMainMenu()
{
	GameModeInterface->ResetGame();
	CurrentWidgetState = EWidgetState::MainMenu;
	ApplyWidgetState(CurrentWidgetState);
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

	if (bShow)
	{
		int32 HighScore = GameModeInterface->GetHighScore();
		GameOverWidget->SetHighScoreText(HighScore);

		int32 CurrentScore = GameModeInterface->GetCurrentScore();
		GameOverWidget->SetCurrentScoreText(CurrentScore);

		int32 CollectedCoins = GameModeInterface->GetCollectedCoins();
		GameOverWidget->SetCoinsCollectedText(CollectedCoins);

		int32 CollectedGems = GameModeInterface->GetCollectedGems() - GameModeInterface->GetGemsSpent();
		GameOverWidget->SetGemsCollectedText(CollectedGems);
	}
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

void UGameWidget::ShowShopWidget()
{
	ShopWidget->SetVisibility(ESlateVisibility::Visible);

	int32 TotalCoins = GameModeInterface->GetTotalCoins();
	ShopWidget->TotalCoinsWidget->UpdateCoinsText(TotalCoins);
	int32 TotalGems = GameModeInterface->GetTotalGems();
	ShopWidget->TotalGemsWidget->UpdateGemsText(TotalGems);
}

void UGameWidget::UpdateCollectiblesUI(bool bCoin)
{
	int32 Total = bCoin ? GameModeInterface->GetTotalCoins() : GameModeInterface->GetTotalGems();
	int32 CollectedCoins = GameModeInterface->GetCollectedCoins();
	int32 TempTotalCoins = Total + CollectedCoins;

	if (bCoin)
		TotalCoinsWidget->UpdateCoinsText(TempTotalCoins);
	else
		TotalGemsWidget->UpdateGemsText(Total);
}

void UGameWidget::PauseGameAfterRewardAD()
{
	TotalGemsWidget->UpdateGemsText(GameModeInterface->GetTotalGems());
	TimeProgressBar->SetPercent(1.0f);
	World->GetTimerManager().UnPauseTimer(GameTimer);
	PauseGame();
}

void UGameWidget::SettingsButtonClicked()
{
	SettingsWidget->SetVisibility(ESlateVisibility::Visible);
}

void UGameWidget::NoAdsButtonClicked()
{
#if PLATFORM_ANDROID
	const FString ProductId = TEXT("removeads");
	ShopWidget->QueryProductDetails(ProductId);
#endif
}

void UGameWidget::HideNoAdsButton()
{
	FUserProgression UserProgression = GameInstanceInterface->GetUserProgression();
	bool bNoAds = UserProgression.bNoAds;
	ESlateVisibility NoAdsButtonVisibility = bNoAds ? ESlateVisibility::Collapsed : ESlateVisibility::Visible;
	NoAdsButton->SetVisibility(NoAdsButtonVisibility);
}

void UGameWidget::EnablePlayButton()
{
	PlayButton->SetIsEnabled(true);
}

void UGameWidget::ApplyWidgetState(EWidgetState State)
{
	int32 TotalCoins = GameModeInterface->GetTotalCoins();
	int32 TotalGems = GameModeInterface->GetTotalGems();

	switch (State)
	{
	case EWidgetState::MainMenu:
		PlayButton->SetVisibility(ESlateVisibility::Visible);
		QuitButton->SetVisibility(ESlateVisibility::Visible);
		ShopButton->SetVisibility(ESlateVisibility::Visible);
		SettingsButton->SetVisibility(ESlateVisibility::Visible);
		HighScoreText->SetVisibility(ESlateVisibility::Visible);
		BouncyBucketsLogo->SetVisibility(ESlateVisibility::Visible);
		HideNoAdsButton();

		GameOverWidget->SetVisibility(ESlateVisibility::Hidden);
		PauseButton->SetVisibility(ESlateVisibility::Hidden);
		TimeProgressBar->SetVisibility(ESlateVisibility::Hidden);
		TotalCoinsWidget->SetVisibility(ESlateVisibility::Hidden);
		TotalGemsWidget->SetVisibility(ESlateVisibility::Hidden);
		ScoreText->SetVisibility(ESlateVisibility::Hidden);
		BlackBorder->SetVisibility(ESlateVisibility::Hidden);
		break;

	case EWidgetState::Playing:
		PlayButton->SetVisibility(ESlateVisibility::Hidden);
		QuitButton->SetVisibility(ESlateVisibility::Hidden);
		ShopButton->SetVisibility(ESlateVisibility::Hidden);
		SettingsButton->SetVisibility(ESlateVisibility::Hidden);
		HighScoreText->SetVisibility(ESlateVisibility::Hidden);
		BouncyBucketsLogo->SetVisibility(ESlateVisibility::Hidden);
		NoAdsButton->SetVisibility(ESlateVisibility::Hidden);

		PauseButton->SetVisibility(ESlateVisibility::Visible);
		TimeProgressBar->SetVisibility(ESlateVisibility::HitTestInvisible);
		TotalCoinsWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
		TotalCoinsWidget->UpdateCoinsText(TotalCoins);
		TotalGemsWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
		TotalGemsWidget->UpdateGemsText(TotalGems);
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