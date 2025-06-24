// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ComboProgressBarWidget.generated.h"

class UImage;
class IGameModeInterface;

UCLASS()
class FLAPPYHOOP_API UComboProgressBarWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:

	void EndTimer();

private:

	IGameModeInterface* GameModeInterface = nullptr;

	virtual void NativeConstruct() override;

	void SetPercent(float Percent);

	UPROPERTY(meta = (BindWidget))
	UImage* ProgressBarImage;

	UMaterialInstanceDynamic* ComboBarMID;

	UPROPERTY(EditDefaultsOnly, Category = "Parameters")
	UMaterialInterface* BaseMaterial;

	FTimerHandle ComboTimer;

	void UpdateProgressBar();

	UWorld* World = nullptr;

	void StartTimer();

	UPROPERTY(EditDefaultsOnly, Category = "Parameters")
	float MaxComboTime = 5.0f;
};
