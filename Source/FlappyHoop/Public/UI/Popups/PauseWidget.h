// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/Common/PopupWidget.h"
#include "PauseWidget.generated.h"

class UButton;
class UWidget;
class UGameWidget;

/** Pause menu: Resume, Home, Settings. Pausing/unpausing the game itself is the UI root's job. */
UCLASS()
class FLAPPYHOOP_API UPauseWidget : public UPopupWidget
{
	GENERATED_BODY()

public:

	void SetGameWidget(UGameWidget* NewGameWidget) { GameWidget = NewGameWidget; }

protected:

	virtual void NativeOnInitialized() override;

private:

	UFUNCTION()
	void OnResumeClicked();

	UFUNCTION()
	void OnHomeClicked();

	UFUNCTION()
	void OnSettingsClicked();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ResumeButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> HomeButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SettingsButton;

	/** Optional: the dark background (was hidden in the old game widget's designer; forced on here). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> BlackBorder;

	UPROPERTY(Transient)
	TObjectPtr<UGameWidget> GameWidget = nullptr;
};
