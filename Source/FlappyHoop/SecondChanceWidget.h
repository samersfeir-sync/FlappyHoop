// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SecondChanceWidget.generated.h"

class UButton;
class UTextBlock;
class UCircularThrobber;

UCLASS()
class FLAPPYHOOP_API USecondChanceWidget : public UUserWidget
{
	GENERATED_BODY()
	
private:

	virtual void NativeConstruct() override;

	void StartSkipTimer();

	void DecreaseTimer();

	UPROPERTY(EditDefaultsOnly);
	int MaxSkipTime = 12;

	UPROPERTY(meta = (BindWidget))
	UButton* WatchAdButton;

	UPROPERTY(meta = (BindWidget))
	UButton* SkipAdButton;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* SkipCountdownText;

	UPROPERTY(meta = (BindWidget))
	UCircularThrobber* LoadingAdThrobber;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* MainText;

	FTimerHandle SkipTimerHandle;

	UFUNCTION()
	void WatchAdButtonClicked();

	UFUNCTION()
	void SkipAdButtonClicked();

	class IGameModeInterface* GameModeInterface = nullptr;

	UFUNCTION()
	void FailedToLoadSecondChanceAd(int ErrorCode, FString ErrorMessage);

	void ChangeMainText(FString NewText, FLinearColor NewColor);
};
