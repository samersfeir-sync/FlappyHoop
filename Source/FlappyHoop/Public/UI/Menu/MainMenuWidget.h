// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/DifficultyTypes.h"
#include "MainMenuWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;
class UGameWidget;

/** Title screen: logo, high score, Play / Shop / Settings / Quit / No Ads. */
UCLASS()
class FLAPPYHOOP_API UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	void SetGameWidget(UGameWidget* NewGameWidget) { GameWidget = NewGameWidget; }

	/** Updates the high score and the No Ads button (call when the menu is shown). */
	void Refresh();

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UImage> BouncyBucketsLogo;

protected:

	/**
	 * Called whenever the selected difficulty is shown (menu opens, or Easy/Hard is pressed).
	 * Implement in BP_MainMenuWidget to style the buttons (the selected one is also disabled from code).
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Difficulty")
	void OnDifficultyShown(EDifficultyMode Mode);

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:

	UFUNCTION() void OnPlayClicked();
	UFUNCTION() void OnQuitClicked();
	UFUNCTION() void OnShopClicked();
	UFUNCTION() void OnSettingsClicked();
	UFUNCTION() void OnNoAdsClicked();
	UFUNCTION() void OnLeaderboardClicked();
	UFUNCTION() void OnEasyClicked();
	UFUNCTION() void OnHardClicked();

	void SelectDifficulty(EDifficultyMode Mode);
	void ShowDifficulty();

	void UpdateNoAdsButton();

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> PlayButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> QuitButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> ShopButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> SettingsButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> NoAdsButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> HighScoreText;

	/**
	 * Optional text on/under the No Ads button. Filled with "Remove Ads <price>" (Google Play's localized price)
	 * so it's clearly an in-app purchase (Families policy). Falls back to NoAdsLabel when no price is known yet.
	 */
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> NoAdsPriceText;

	/** Optional: opens the Google Play Games / Game Center leaderboard. Hidden where neither exists (editor, desktop). */
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> LeaderboardButton;

	/** Optional difficulty pickers. The selected one is disabled so it reads as "chosen". */
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> EasyButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> HardButton;

	/** Optional: "Easy" / "Hard". HighScoreText always shows the selected mode's best. */
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> DifficultyText;

	UPROPERTY(EditDefaultsOnly, Category = "Difficulty")
	FText EasyLabel = NSLOCTEXT("MainMenu", "Easy", "Easy");

	UPROPERTY(EditDefaultsOnly, Category = "Difficulty")
	FText HardLabel = NSLOCTEXT("MainMenu", "Hard", "Hard");

	UPROPERTY(EditDefaultsOnly, Category = "No Ads")
	FText NoAdsLabel = NSLOCTEXT("MainMenu", "NoAdsLabel", "Remove Ads");

	void UpdateNoAdsPrice();

	UPROPERTY(Transient)
	TObjectPtr<UGameWidget> GameWidget = nullptr;
};
