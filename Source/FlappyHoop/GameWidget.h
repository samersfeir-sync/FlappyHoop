// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameWidget.generated.h"

class UButton;
class UTextBlock;
class UProgressBar;
class UBorder;
class IGameModeInterface;
class UImage;
class UTotalCoinsWidget;
class IGameInstanceInterface;
class USecondChanceWidget;
class USettingsWidget;
class UShopWidget;

UENUM(BlueprintType)
enum class EWidgetState : uint8
{
	MainMenu	UMETA(DisplayName = "Main Menu"),
	Playing		UMETA(DisplayName = "Playing"),
};

UCLASS()
class FLAPPYHOOP_API UGameWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	void SetWorldReference(UWorld* NewWorld) { World = NewWorld; }

	UFUNCTION()
	void QuitGame();

	UFUNCTION()
	void ReturnToMainMenu();

	void ShowGameOverWidget(bool bShow);

	void EndComboTimer();

	void ShowComboText();

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UImage* BouncyBucketsLogo;

	UFUNCTION()
	void PauseGame();

	EWidgetState GetCurrentWidgetState() const { return CurrentWidgetState; }

	void ShowSecondChanceWidget(bool bShow);

	UPROPERTY(meta = (BindWidget))
	USecondChanceWidget* SecondChanceWidget;

private:

	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	UButton* PlayButton;
	
	UPROPERTY(meta = (BindWidget))
	UButton* QuitButton;

	UPROPERTY(meta = (BindWidget))
	UButton* ShopButton;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* HighScoreText;

	UPROPERTY(meta = (BindWidget))
	UButton* PauseButton;

	UPROPERTY(meta = (BindWidget))
	UButton* HomeButton;

	UPROPERTY(meta = (BindWidget))
	UButton* ResumeButton;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* TimeProgressBar;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* ScoreText;

	UPROPERTY(meta = (BindWidget))
	UBorder* BlackBorder;

	UPROPERTY(meta = (BindWidget))
	UButton* SettingsButton;

	UFUNCTION()
	void OnPlayClicked();

	UFUNCTION()
	void ResumeGame();

	void EnablePlayButton();

	IGameModeInterface* GameModeInterface = nullptr;

	IGameInstanceInterface* GameInstanceInterface = nullptr;

	UWorld* World = nullptr;

	void ApplyWidgetState(EWidgetState State);

	FTimerHandle GameTimer;

	void StartTimer();
	void UpdateProgressBar();

	float ProgressStep;
	float TickInterval = 0.01f;

	void OnPointScored();
	
	void UpdateScoreUI(int NewScore);

	UPROPERTY(meta = (BindWidget))
	class UGameOverWidget* GameOverWidget;

	UPROPERTY(meta = (BindWidget))
	class UComboProgressBarWidget* ComboProgressBarWidget;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* ComboText;

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	UWidgetAnimation* ComboTextAnimation;

	UFUNCTION()
	void OnComboAnimationFinished();

	void UpdateHighScoreUI();

	UPROPERTY(meta = (BindWidget))
	UTotalCoinsWidget* TotalCoinsWidget;

	UPROPERTY(meta = (BindWidget))
	UShopWidget* ShopWidget;

	UFUNCTION()
	void ShowShopWidget();

	void UpdateCoinUI();

	EWidgetState CurrentWidgetState = EWidgetState::MainMenu;

	void PauseGameAfterRewardAD();

	UPROPERTY(meta = (BindWidget))
	USettingsWidget* SettingsWidget;

	UFUNCTION()
	void SettingsButtonClicked();
};
