// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/GameWidget.h"
#include "Framework/BaseGameMode.h"
#include "UI/HUD/HUDWidget.h"
#include "UI/Menu/MainMenuWidget.h"
#include "UI/Popups/GameOverWidget.h"
#include "UI/Popups/PauseWidget.h"
#include "UI/Popups/QuitConfirmWidget.h"
#include "UI/Popups/SecondChanceWidget.h"
#include "UI/Popups/SettingsWidget.h"
#include "UI/Shop/ShopWidget.h"
#include "UI/Shop/InsufficientCoinsWidget.h"
#include "Misc/CoreDelegates.h"
#include "Components/CanvasPanelSlot.h"
#include "Core/FlappyHoop.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"

void UGameWidget::NativeConstruct()
{
	Super::NativeConstruct();

	MainMenuWidget->SetGameWidget(this);
	HUDWidget->SetGameWidget(this);
	PauseWidget->SetGameWidget(this);
	GameOverWidget->SetGameWidget(this);
	if (QuitConfirmWidget)
	{
		QuitConfirmWidget->SetGameWidget(this);
	}

	FCoreDelegates::ApplicationWillDeactivateDelegate.AddUObject(this, &UGameWidget::OnAppDeactivated);
	FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddUObject(this, &UGameWidget::OnAppDeactivated);

	GameMode = ABaseGameMode::Get(this);
	if (GameMode)
	{
		GameMode->OnRunPhaseChanged.AddUObject(this, &UGameWidget::OnRunPhaseChanged);
		GameMode->OnSecondChanceGranted.AddUObject(this, &UGameWidget::OnSecondChanceGranted);
	}

	const bool bInMenu = !GameMode || GameMode->GetCurrentGameState() == EGameStateEnum::MainMenu;
	ShowMenu(bInMenu);
}

void UGameWidget::NativeDestruct()
{
	FCoreDelegates::ApplicationWillDeactivateDelegate.RemoveAll(this);
	FCoreDelegates::ApplicationWillEnterBackgroundDelegate.RemoveAll(this);

	if (GameMode)
	{
		GameMode->OnRunPhaseChanged.RemoveAll(this);
		GameMode->OnSecondChanceGranted.RemoveAll(this);
	}

	Super::NativeDestruct();
}

// ---------------------------------------------------------------------------------------------------------------------
// Run phases
// ---------------------------------------------------------------------------------------------------------------------

void UGameWidget::OnRunPhaseChanged(EGameStateEnum OldPhase, EGameStateEnum NewPhase)
{
	switch (NewPhase)
	{
	case EGameStateEnum::MainMenu:
		CloseRunPopups();
		ShowMenu(true);
		break;

	case EGameStateEnum::Playing:
		SecondChanceWidget->Close();
		if (OldPhase == EGameStateEnum::MainMenu)
		{
			ShowMenu(false);
		}
		break;

	case EGameStateEnum::SecondChance:
		SecondChanceWidget->Open();
		break;

	case EGameStateEnum::GameOver:
		SecondChanceWidget->Close();
		GameOverWidget->Open();
		break;

	default:
		break;
	}
}

void UGameWidget::ShowMenu(bool bMenu)
{
	// Menu and HUD are screens, not popups: they swap instantly.
	MainMenuWidget->SetVisibility(bMenu ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	HUDWidget->SetVisibility(bMenu ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);

	if (bMenu)
	{
		MainMenuWidget->Refresh();
	}
	HUDWidget->ResetForNewRun();
}

void UGameWidget::OnAppDeactivated()
{
	// Only a live run; the menu, second chance (the rewarded ad itself backgrounds the app) and game over are left alone.
	if (GameMode && GameMode->GetCurrentGameState() == EGameStateEnum::Playing && !PauseWidget->IsOpen())
	{
		PauseGame();
	}
}

void UGameWidget::HandleBackButton()
{
	// Top-most popup first (Settings can sit on top of Pause).
	if (SettingsWidget->IsOpen())
	{
		SettingsWidget->Close();
		return;
	}

	if (ShopWidget->IsOpen())
	{
		if (ShopWidget->InsufficientCoinsWidget && ShopWidget->InsufficientCoinsWidget->IsOpen())
		{
			ShopWidget->InsufficientCoinsWidget->Close();
		}
		else
		{
			ShopWidget->Close();
		}
		return;
	}

	if (QuitConfirmWidget && QuitConfirmWidget->IsOpen())
	{
		QuitConfirmWidget->Close();
		return;
	}

	if (PauseWidget->IsOpen())
	{
		ResumeGame();
		return;
	}

	if (GameOverWidget->IsOpen())
	{
		ReturnToMainMenu();
		return;
	}

	if (SecondChanceWidget->IsOpen())
	{
		return; // the player has to pick continue or skip; Back shouldn't throw the run away
	}

	const EGameStateEnum Phase = GameMode ? GameMode->GetCurrentGameState() : EGameStateEnum::MainMenu;
	if (Phase == EGameStateEnum::Playing)
	{
		PauseGame();
	}
	else if (Phase == EGameStateEnum::MainMenu)
	{
		RequestQuit();
	}
}

void UGameWidget::RequestQuit()
{
	if (QuitConfirmWidget)
	{
		// Always draw above whatever asked (menu, Game Over), whatever the hierarchy order is.
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(QuitConfirmWidget->Slot))
		{
			CanvasSlot->SetZOrder(1000);
		}
		UE_LOG(LogFlappyHoop, Log, TEXT("UI: quit confirm opened"));
		QuitConfirmWidget->Open();
	}
	else
	{
		UE_LOG(LogFlappyHoop, Warning, TEXT("UI: no QuitConfirmWidget in the game widget, quitting directly"));
		QuitGame();
	}
}

void UGameWidget::CloseRunPopups()
{
	PauseWidget->CloseImmediately();
	SettingsWidget->CloseImmediately(); // may have been opened from the pause menu
	SecondChanceWidget->CloseImmediately();
	GameOverWidget->CloseImmediately();
}

void UGameWidget::OnSecondChanceGranted()
{
	PauseGame();
}

// ---------------------------------------------------------------------------------------------------------------------
// Actions (called by the menu, HUD and popups)
// ---------------------------------------------------------------------------------------------------------------------

void UGameWidget::QuitGame()
{
	UKismetSystemLibrary::QuitGame(this, UGameplayStatics::GetPlayerController(this, 0), EQuitPreference::Quit, true);
}

void UGameWidget::ReturnToMainMenu()
{
	if (GameMode)
	{
		GameMode->ReturnToMenu(); // unpauses; the UI follows through OnRunPhaseChanged
	}
}

void UGameWidget::PauseGame()
{
	UGameplayStatics::SetGamePaused(this, true);
	PauseWidget->Open();
}

void UGameWidget::ResumeGame()
{
	SettingsWidget->Close();
	PauseWidget->Close();
	UGameplayStatics::SetGamePaused(this, false);
}

void UGameWidget::OpenSettings()
{
	SettingsWidget->Open();
}

void UGameWidget::OpenShop()
{
	ShopWidget->Open();
}
