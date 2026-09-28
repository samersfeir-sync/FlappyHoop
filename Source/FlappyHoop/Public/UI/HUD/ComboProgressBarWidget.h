// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/GameStateEnum.h"
#include "ComboProgressBarWidget.generated.h"

class UImage;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class ABaseGameMode;

UCLASS()
class FLAPPYHOOP_API UComboProgressBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	void EndTimer();

protected:

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:

	UPROPERTY(Transient)
	TObjectPtr<ABaseGameMode> GameMode = nullptr;

	void OnRunPhaseChanged(EGameStateEnum OldPhase, EGameStateEnum NewPhase);
	void SetPercent(float Percent);
	void StartTimer();
	void UpdateProgressBar();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ProgressBarImage;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ComboBarMID;

	UPROPERTY(EditDefaultsOnly, Category = "Parameters")
	TObjectPtr<UMaterialInterface> BaseMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Parameters", meta = (ClampMin = "0.1"))
	float MaxComboTime = 5.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Parameters", meta = (ClampMin = "0.005"))
	float TickInterval = 0.01f;

	float CurrentPercent = 1.0f;
	FTimerHandle ComboTimer;
};
