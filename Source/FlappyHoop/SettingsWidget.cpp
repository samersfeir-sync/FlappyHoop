// Fill out your copyright notice in the Description page of Project Settings.


#include "SettingsWidget.h"
#include "Components/Button.h"
#include "GameInstanceInterface.h"
#include "FunctionsLibrary.h"
#include "Components/Border.h"
#include "Sound/SoundClass.h"

void USettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	GameInstanceInterface = UFunctionsLibrary::GetGameInstanceInterface(this);

	if (GameInstanceInterface)
	{
		UserProgression = GameInstanceInterface->GetUserProgression();

		bool bMutedSFX = UserProgression.bIsSFXMuted;
		UpdateBordersVisuals(MutedSFXBorder, UnmutedSFXBorder, bMutedSFX);
		SetSoundClassVolume(SFXSoundClass, !bMutedSFX ? 1.0f : 0.0f);

		bool bMutedMusic = UserProgression.bIsMusicMuted;
		UpdateBordersVisuals(MutedMusicBorder, UnmutedMusicBorder, bMutedMusic);
		SetSoundClassVolume(MusicSoundClass, !bMutedMusic ? 1.0f : 0.0f);
	}

	BackButton->OnClicked.AddDynamic(this, &USettingsWidget::BackButtonClicked);
	SoundEffectButton->OnClicked.AddDynamic(this, &USettingsWidget::ToggleSoundEffects);
	MusicButton->OnClicked.AddDynamic(this, &USettingsWidget::ToggleMusic);
}

void USettingsWidget::BackButtonClicked()
{
	SetVisibility(ESlateVisibility::Hidden);
}

void USettingsWidget::UpdateBordersVisuals(UBorder* MutedBorder, UBorder* UnmutedBorder, bool bMuted) const
{
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
}

void USettingsWidget::ToggleSoundEffects()
{
	if (!GameInstanceInterface) return;

	bool bMuted = UserProgression.bIsSFXMuted;
	UserProgression.bIsSFXMuted = !bMuted;
	UpdateBordersVisuals(MutedSFXBorder, UnmutedSFXBorder, !bMuted);
	SetSoundClassVolume(SFXSoundClass, bMuted ? 1.0f : 0.0f);
	GameInstanceInterface->SaveUserProgression(UserProgression);
}
	
void USettingsWidget::ToggleMusic()
{
	if (!GameInstanceInterface) return;

	bool bMuted = UserProgression.bIsMusicMuted;
	UserProgression.bIsMusicMuted = !bMuted;
	UpdateBordersVisuals(MutedMusicBorder, UnmutedMusicBorder, !bMuted);
	SetSoundClassVolume(MusicSoundClass, !bMuted ? 1.0f : 0.0f);
	GameInstanceInterface->SaveUserProgression(UserProgression);
}

void USettingsWidget::SetSoundClassVolume(USoundClass* SoundClass, float Volume)
{
	SoundClass->Properties.Volume = Volume;
}
