// Fill out your copyright notice in the Description page of Project Settings.


#include "ComboProgressBarWidget.h"
#include "Components/Image.h"
#include "GameModeInterface.h"
#include "FunctionsLibrary.h"

void UComboProgressBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	World = GetWorld();

	ComboBarMID = UMaterialInstanceDynamic::Create(BaseMaterial, this);
	ProgressBarImage->SetBrushFromMaterial(ComboBarMID);

	GameModeInterface = UFunctionsLibrary::GetGameModeInterface(this);

	if (GameModeInterface)
	{
		GameModeInterface->OnPointScoredDelegate().AddUObject(this, &UComboProgressBarWidget::StartTimer);
	}
}


void UComboProgressBarWidget::SetPercent(float Percent)
{
	if (ComboBarMID)
	{
		ComboBarMID->SetScalarParameterValue("Percentage", FMath::Clamp(Percent, 0.0f, 1.0f));
	}
}

void UComboProgressBarWidget::StartTimer()
{
	if (ComboTimer.IsValid())
	{
		SetPercent(1.0f);
		return;
	}

	World->GetTimerManager().SetTimer(ComboTimer, this, &UComboProgressBarWidget::UpdateProgressBar, 0.01f, true);
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UComboProgressBarWidget::EndTimer()
{
	GameModeInterface->ResetScoreMultiplier();
	SetPercent(1.0f);
	World->GetTimerManager().ClearTimer(ComboTimer);
	SetVisibility(ESlateVisibility::Hidden);
}

void UComboProgressBarWidget::UpdateProgressBar()
{
	float CurrentPercent = ComboBarMID->K2_GetScalarParameterValue("Percentage");
	float ProgressStep = 0.01f / MaxComboTime;
	CurrentPercent -= ProgressStep;
	SetPercent(CurrentPercent);

	if (CurrentPercent <= 0.0f)
	{
		CurrentPercent = 1.0f;
		EndTimer();
	}
}