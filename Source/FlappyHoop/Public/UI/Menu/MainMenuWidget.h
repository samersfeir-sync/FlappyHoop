// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
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

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:

	UFUNCTION() void OnPlayClicked();
	UFUNCTION() void OnQuitClicked();
	UFUNCTION() void OnShopClicked();
	UFUNCTION() void OnSettingsClicked();
	UFUNCTION() void OnNoAdsClicked();

	void UpdateNoAdsButton();

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> PlayButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> QuitButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> ShopButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> SettingsButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> NoAdsButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> HighScoreText;

	UPROPERTY(Transient)
	TObjectPtr<UGameWidget> GameWidget = nullptr;
};
