// Fill out your copyright notice in the Description page of Project Settings.


#include "SettingsWidget.h"
#include "Components/Button.h"
#include "GameInstanceInterface.h"
#include "FunctionsLibrary.h"
#include "Components/Border.h"

void USettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	GameInstanceInterface = UFunctionsLibrary::GetGameInstanceInterface(this);

	BackButton->OnClicked.AddDynamic(this, &USettingsWidget::BackButtonClicked);
	SoundEffectButton->OnClicked.AddDynamic(this, &USettingsWidget::ToggleSoundEffects);
}

void USettingsWidget::BackButtonClicked()
{
	SetVisibility(ESlateVisibility::Hidden);
}

FUserProgression USettingsWidget::UpdateBordersVisuals(UBorder* MutedBorder, UBorder* UnmutedBorder)
{
	FUserProgression UserProgression = GameInstanceInterface->GetUserProgression();
	const bool bMuted = UserProgression.bIsSFXMuted;

	const float MutedAlpha = bMuted ? 1.0f : 0.0f;
	const float UnmutedAlpha = bMuted ? 0.0f : 1.0f;

	FLinearColor FinalMutedColor = MutedColor;   FinalMutedColor.A = MutedAlpha;
	FLinearColor FinalUnmutedColor = UnmutedColor; FinalUnmutedColor.A = UnmutedAlpha;

	if (MutedBorder)
	{
		MutedBorder->SetBrushColor(FinalMutedColor);

		FSlateBrush MutedBrush = MutedBorder->Background;
		MutedBrush.OutlineSettings.Color = FLinearColor(0, 0, 0, MutedAlpha);
		MutedBorder->SetBrush(MutedBrush);
	}

	if (UnmutedBorder)
	{
		UnmutedBorder->SetBrushColor(FinalUnmutedColor);

		FSlateBrush UnmutedBrush = UnmutedBorder->Background;
		UnmutedBrush.OutlineSettings.Color = FLinearColor(0, 0, 0, UnmutedAlpha);
		UnmutedBorder->SetBrush(UnmutedBrush);
	}

	return UserProgression;
}

void USettingsWidget::ToggleSoundEffects()
{
	if (!GameInstanceInterface) return;
	bool bMuted = UserProgression.bIsSFXMuted;
	UserProgression.bIsSFXMuted = !bMuted;

	FUserProgression UserProgression = UpdateBordersVisuals(MutedSFXBorder, UnmutedSFXBorder);
	GameInstanceInterface->SaveUserProgression(UserProgression);
}
