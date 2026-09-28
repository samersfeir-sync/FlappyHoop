// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/Common/PopupWidget.h"
#include "QuitConfirmWidget.generated.h"

class UButton;
class UGameWidget;

/** "Quit the game?" Shown when Back is pressed on the main menu. */
UCLASS()
class FLAPPYHOOP_API UQuitConfirmWidget : public UPopupWidget
{
	GENERATED_BODY()

public:

	void SetGameWidget(UGameWidget* NewGameWidget) { GameWidget = NewGameWidget; }

protected:

	virtual void NativeOnInitialized() override;

private:

	UFUNCTION()
	void OnYesClicked();

	UFUNCTION()
	void OnNoClicked();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> YesButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> NoButton;

	UPROPERTY(Transient)
	TObjectPtr<UGameWidget> GameWidget = nullptr;
};
