// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/Menu/MainMenuWidget.h"
#include "UI/HUD/GameWidget.h"
#include "Core/FunctionsLibrary.h"
#include "Framework/BaseGameMode.h"
#include "Systems/SaveSubsystem.h"
#include "Systems/StoreSubsystem.h"
#include "Systems/LeaderboardSubsystem.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

void UMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	PlayButton->OnClicked.AddUniqueDynamic(this, &UMainMenuWidget::OnPlayClicked);
	if (QuitButton) QuitButton->OnClicked.AddUniqueDynamic(this, &UMainMenuWidget::OnQuitClicked);
	ShopButton->OnClicked.AddUniqueDynamic(this, &UMainMenuWidget::OnShopClicked);
	SettingsButton->OnClicked.AddUniqueDynamic(this, &UMainMenuWidget::OnSettingsClicked);
	NoAdsButton->OnClicked.AddUniqueDynamic(this, &UMainMenuWidget::OnNoAdsClicked);

	if (LeaderboardButton)
	{
		LeaderboardButton->OnClicked.AddUniqueDynamic(this, &UMainMenuWidget::OnLeaderboardClicked);
		// Always shown (also in the editor/desktop, where pressing it does nothing) so the layout can be checked anywhere.
		LeaderboardButton->SetVisibility(ESlateVisibility::Visible);
	}

	if (LeaderboardModeText)
	{
		LeaderboardModeText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (EasyButton) EasyButton->OnClicked.AddUniqueDynamic(this, &UMainMenuWidget::OnEasyClicked);
	if (HardButton) HardButton->OnClicked.AddUniqueDynamic(this, &UMainMenuWidget::OnHardClicked);
	if (DifficultyText) DifficultyText->SetVisibility(ESlateVisibility::HitTestInvisible);

	if (ABaseGameMode* GameMode = ABaseGameMode::Get(this))
	{
		// Also covers the SetDifficulty console command.
		GameMode->OnDifficultyChanged.AddWeakLambda(this, [this](EDifficultyMode) { ShowDifficulty(); });
	}

	// Same as the HUD: the menu as a whole is shown/hidden by the UI root, so its pieces are always on.
	PlayButton->SetVisibility(ESlateVisibility::Visible);
	if (QuitButton) QuitButton->SetVisibility(ESlateVisibility::Visible);
	ShopButton->SetVisibility(ESlateVisibility::Visible);
	SettingsButton->SetVisibility(ESlateVisibility::Visible);
	HighScoreText->SetVisibility(ESlateVisibility::HitTestInvisible);
	BouncyBucketsLogo->SetVisibility(ESlateVisibility::HitTestInvisible);

	if (UStoreSubsystem* Store = UStoreSubsystem::Get(this))
	{
		Store->OnAdsRemoved.AddUObject(this, &UMainMenuWidget::UpdateNoAdsButton);
		Store->OnPricesUpdated.AddUObject(this, &UMainMenuWidget::UpdateNoAdsPrice);
	}

	Refresh();
}

void UMainMenuWidget::NativeDestruct()
{
	if (UStoreSubsystem* Store = UStoreSubsystem::Get(this))
	{
		Store->OnAdsRemoved.RemoveAll(this);
		Store->OnPricesUpdated.RemoveAll(this);
	}

	if (ABaseGameMode* GameMode = ABaseGameMode::Get(this))
	{
		GameMode->OnDifficultyChanged.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UMainMenuWidget::Refresh()
{
	ShowDifficulty();
	UpdateNoAdsButton();
	UpdateNoAdsPrice();
}

void UMainMenuWidget::ShowDifficulty()
{
	const ABaseGameMode* GameMode = ABaseGameMode::Get(this);
	if (!GameMode)
	{
		return;
	}

	const EDifficultyMode Mode = GameMode->GetDifficulty();
	UFunctionsLibrary::SetHighScoreText(HighScoreText, GameMode->GetHighScore(Mode));

	if (EasyButton) EasyButton->SetIsEnabled(Mode != EDifficultyMode::Easy);
	if (HardButton) HardButton->SetIsEnabled(Mode != EDifficultyMode::Hard);
	if (DifficultyText) DifficultyText->SetText(Mode == EDifficultyMode::Hard ? HardLabel : EasyLabel);
	if (LeaderboardModeText) LeaderboardModeText->SetText(Mode == EDifficultyMode::Hard ? LeaderboardHardLabel : LeaderboardEasyLabel);

	OnDifficultyShown(Mode);
}

void UMainMenuWidget::SelectDifficulty(EDifficultyMode Mode)
{
	if (ABaseGameMode* GameMode = ABaseGameMode::Get(this))
	{
		GameMode->SetDifficulty(Mode);
	}
	ShowDifficulty();
}

void UMainMenuWidget::OnEasyClicked()
{
	SelectDifficulty(EDifficultyMode::Easy);
}

void UMainMenuWidget::OnHardClicked()
{
	SelectDifficulty(EDifficultyMode::Hard);
}

void UMainMenuWidget::UpdateNoAdsPrice()
{
	if (!NoAdsPriceText)
	{
		return;
	}

	const UStoreSubsystem* Store = UStoreSubsystem::Get(this);
	const FString Price = Store ? Store->GetLocalizedPrice(FlappyHoopStore::RemoveAdsProductId) : FString();

	NoAdsPriceText->SetText(Price.IsEmpty()
		? NoAdsLabel
		: FText::Format(NSLOCTEXT("MainMenu", "NoAdsWithPrice", "{0} {1}"), NoAdsLabel, FText::FromString(Price)));
}

void UMainMenuWidget::UpdateNoAdsButton()
{
	const USaveSubsystem* Save = USaveSubsystem::Get(this);
	const bool bNoAds = Save && Save->GetProgression().bNoAds;
	NoAdsButton->SetVisibility(bNoAds ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
}

void UMainMenuWidget::OnPlayClicked()
{
	if (ABaseGameMode* GameMode = ABaseGameMode::Get(this))
	{
		GameMode->StartRun(); // the UI root switches to the HUD on the phase change
	}
}

void UMainMenuWidget::OnQuitClicked()
{
	if (GameWidget)
	{
		GameWidget->RequestQuit();
	}
}

void UMainMenuWidget::OnShopClicked()
{
	if (GameWidget)
	{
		GameWidget->OpenShop();
	}
}

void UMainMenuWidget::OnSettingsClicked()
{
	if (GameWidget)
	{
		GameWidget->OpenSettings();
	}
}

void UMainMenuWidget::OnNoAdsClicked()
{
	if (UStoreSubsystem* Store = UStoreSubsystem::Get(this))
	{
		Store->Purchase(FlappyHoopStore::RemoveAdsProductId);
	}
}

void UMainMenuWidget::OnLeaderboardClicked()
{
	if (ULeaderboardSubsystem* Leaderboards = ULeaderboardSubsystem::Get(this))
	{
		// The board of the selected difficulty; signs in first if needed.
		const ABaseGameMode* GameMode = ABaseGameMode::Get(this);
		Leaderboards->ShowLeaderboard(GameMode ? GameMode->GetDifficulty() : EDifficultyMode::Easy);
	}
}
