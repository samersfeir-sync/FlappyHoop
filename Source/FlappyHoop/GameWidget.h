// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameWidget.generated.h"

class UButton;
class UTextBlock;
class UProgressBar;
class UHorizontalBox;
class IGameModeInterface;

UCLASS()
class FLAPPYHOOP_API UGameWidget : public UUserWidget
{
	GENERATED_BODY()

protected:

	virtual void NativeConstruct() override;

private:

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
	UTextBlock* CoinsText;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* TimeProgressBar;

	UPROPERTY(meta = (BindWidget))
	UHorizontalBox* CoinsBox;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* ScoreText;

	UFUNCTION()
	void OnPlayClicked();

	UFUNCTION()
	void QuitGame();

	void EnablePlayButton();

	IGameModeInterface* GameModeInterface = nullptr;
};
