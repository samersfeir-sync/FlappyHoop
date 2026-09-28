// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/ComboProgressBarWidget.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "Framework/BaseGameMode.h"

static const FName ComboPercentParam(TEXT("Percent"));

void UComboProgressBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (BaseMaterial)
	{
		ComboBarMID = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		ProgressBarImage->SetBrushFromMaterial(ComboBarMID);
	}

	GameMode = ABaseGameMode::Get(this);
	if (GameMode)
	{
		GameMode->OnPointScored.AddUObject(this, &UComboProgressBarWidget::StartTimer);
		GameMode->OnRunPhaseChanged.AddUObject(this, &UComboProgressBarWidget::OnRunPhaseChanged);
	}
}

void UComboProgressBarWidget::NativeDestruct()
{
	if (GameMode)
	{
		GameMode->OnPointScored.RemoveAll(this);
		GameMode->OnRunPhaseChanged.RemoveAll(this);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ComboTimer);
	}

	Super::NativeDestruct();
}

void UComboProgressBarWidget::OnRunPhaseChanged(EGameStateEnum OldPhase, EGameStateEnum NewPhase)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// The combo is frozen while the player decides on a second chance, and ends with the run.
	if (NewPhase == EGameStateEnum::SecondChance)
	{
		World->GetTimerManager().PauseTimer(ComboTimer);
	}
	else if (NewPhase == EGameStateEnum::Playing && OldPhase == EGameStateEnum::SecondChance)
	{
		World->GetTimerManager().UnPauseTimer(ComboTimer);
	}
	else if (NewPhase == EGameStateEnum::GameOver || NewPhase == EGameStateEnum::MainMenu)
	{
		EndTimer();
	}
}

void UComboProgressBarWidget::SetPercent(float Percent)
{
	CurrentPercent = FMath::Clamp(Percent, 0.0f, 1.0f);

	if (ComboBarMID)
	{
		ComboBarMID->SetScalarParameterValue(ComboPercentParam, CurrentPercent);
	}
}

void UComboProgressBarWidget::StartTimer()
{
	SetPercent(1.0f);

	FTimerManager& TimerManager = GetWorld()->GetTimerManager();
	if (!TimerManager.IsTimerActive(ComboTimer))
	{
		TimerManager.SetTimer(ComboTimer, this, &UComboProgressBarWidget::UpdateProgressBar, TickInterval, true);
	}

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UComboProgressBarWidget::EndTimer()
{
	if (GameMode)
	{
		GameMode->ResetScoreMultiplier();
	}

	SetPercent(1.0f);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ComboTimer);
	}

	SetVisibility(ESlateVisibility::Hidden);
}

void UComboProgressBarWidget::UpdateProgressBar()
{
	SetPercent(CurrentPercent - TickInterval / MaxComboTime);

	if (CurrentPercent <= 0.0f)
	{
		EndTimer();
	}
}
