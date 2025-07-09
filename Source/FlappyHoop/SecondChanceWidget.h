// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SecondChanceWidget.generated.h"

class UButton;
class UTextBlock;
class UCircularThrobber;
class URichTextBlock;

UCLASS()
class FLAPPYHOOP_API USecondChanceWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:

	void StartSkipTimer();

	void ResetWidget();

	UFUNCTION()
	void RewardAdFailed(int ErrorCode, FString ErrorMessage);

	UPROPERTY(meta = (BindWidget))
	UButton* WatchAdButton;

	UPROPERTY(meta = (BindWidget))
	UButton* GemButton;

	void ChangeMainText(FString NewText, bool bErrorText);

	UPROPERTY(meta = (BindWidget))
	class UTotalGemsWidget* TotalGemsWidget;

private:

	virtual void NativeConstruct() override;

	void DecreaseTimer();

	UPROPERTY(EditDefaultsOnly);
	int MaxSkipTime = 12;

	UPROPERTY(meta = (BindWidget))
	UButton* SkipAdButton;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* SkipCountdownText;

	UPROPERTY(meta = (BindWidget))
	UCircularThrobber* LoadingAdThrobber;

	FTimerHandle SkipTimerHandle;

	UFUNCTION()
	void WatchAdButtonClicked();

	UFUNCTION()
	void SkipAdButtonClicked();

	UFUNCTION()
	void GemButtonClicked();

	class IGameModeInterface* GameModeInterface = nullptr;

	UPROPERTY(meta = (BindWidget))
	URichTextBlock* MainText;
};
