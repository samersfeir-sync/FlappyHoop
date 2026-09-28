// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/GameStateEnum.h"
#include "GameWidget.generated.h"

class UMainMenuWidget;
class UHUDWidget;
class UPauseWidget;
class USecondChanceWidget;
class USettingsWidget;
class UShopWidget;
class UGameOverWidget;
class UQuitConfirmWidget;
class ABaseGameMode;

/**
 * UI root (the one widget the game mode adds to the viewport).
 * Holds the main menu, the in-run HUD and every popup, and switches between them on run-phase changes.
 * Popups draw in the order they're listed in the Blueprint hierarchy (lower = on top), so keep
 * Settings below Pause, and Shop/Settings below the main menu.
 */
UCLASS()
class FLAPPYHOOP_API UGameWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/** Quits right away (the Yes button of the quit popup). */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void QuitGame();

	/** Asks "Quit the game?" first (Quit button, Back on the menu); quits directly if there's no QuitConfirmWidget. */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void RequestQuit();

	/** Home / Restart: ends the run and goes back to the menu. */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ReturnToMainMenu();

	/** Pauses the game and opens the pause menu. */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void PauseGame();

	/** Closes the pause menu and unpauses. */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ResumeGame();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void OpenSettings();

	UFUNCTION(BlueprintCallable, Category = "UI")
	void OpenShop();

	/**
	 * Android Back (Backspace on desktop): closes the top popup; in a run it pauses;
	 * on the main menu it asks to quit (or quits, if there's no QuitConfirmWidget).
	 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void HandleBackButton();

protected:

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UMainMenuWidget> MainMenuWidget;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UHUDWidget> HUDWidget;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UPauseWidget> PauseWidget;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UGameOverWidget> GameOverWidget;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<USecondChanceWidget> SecondChanceWidget;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<USettingsWidget> SettingsWidget;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UShopWidget> ShopWidget;

	/** Optional "Quit the game?" popup for Back on the main menu. */
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UQuitConfirmWidget> QuitConfirmWidget;

	void OnRunPhaseChanged(EGameStateEnum OldPhase, EGameStateEnum NewPhase);

	/** After a second chance: pause so the player gets a "Resume" moment to get ready. */
	void OnSecondChanceGranted();

	/** App lost focus / went to the background (call, home button, app switch): pause a running game. */
	void OnAppDeactivated();

	void ShowMenu(bool bMenu);
	void CloseRunPopups();

	UPROPERTY(Transient)
	TObjectPtr<ABaseGameMode> GameMode = nullptr;
};
