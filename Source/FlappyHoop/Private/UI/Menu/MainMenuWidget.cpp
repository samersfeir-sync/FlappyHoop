// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/Menu/MainMenuWidget.h"
#include "UI/HUD/GameWidget.h"
#include "Core/FunctionsLibrary.h"
#include "Framework/BaseGameMode.h"
#include "Systems/SaveSubsystem.h"
#include "Systems/StoreSubsystem.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

void UMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	PlayButton->OnClicked.AddUniqueDynamic(this, &UMainMenuWidget::OnPlayClicked);
	QuitButton->OnClicked.AddUniqueDynamic(this, &UMainMenuWidget::OnQuitClicked);
	ShopButton->OnClicked.AddUniqueDynamic(this, &UMainMenuWidget::OnShopClicked);
	SettingsButton->OnClicked.AddUniqueDynamic(this, &UMainMenuWidget::OnSettingsClicked);
	NoAdsButton->OnClicked.AddUniqueDynamic(this, &UMainMenuWidget::OnNoAdsClicked);

	// Same as the HUD: the menu as a whole is shown/hidden by the UI root, so its pieces are always on.
	PlayButton->SetVisibility(ESlateVisibility::Visible);
	QuitButton->SetVisibility(ESlateVisibility::Visible);
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

	Super::NativeDestruct();
}

void UMainMenuWidget::Refresh()
{
	if (const ABaseGameMode* GameMode = ABaseGameMode::Get(this))
	{
		UFunctionsLibrary::SetHighScoreText(HighScoreText, GameMode->GetHighScore());
	}

	UpdateNoAdsButton();
	UpdateNoAdsPrice();
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
