// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Popups/SettingsWidget.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"
#include "Systems/SaveSubsystem.h"
#include "Systems/FlappyAudioSubsystem.h"
#include "Systems/AdsSubsystem.h"

void USettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (const USaveSubsystem* Save = USaveSubsystem::Get(this))
	{
		const FUserProgression& UserProgression = Save->GetProgression();

		UpdateBordersVisuals(MutedSFXBorder, UnmutedSFXBorder, UserProgression.bIsSFXMuted);
		ApplySFXVolume(UserProgression.bIsSFXMuted ? 0.0f : 1.0f);

		// Only reflect the saved state visually here; starting the music is the game instance's job.
		UpdateBordersVisuals(MutedMusicBorder, UnmutedMusicBorder, UserProgression.bIsMusicMuted);
		if (UserProgression.bIsMusicMuted)
		{
			ApplyMusicMuted(true);
		}
	}

	BackButton->OnClicked.AddUniqueDynamic(this, &USettingsWidget::BackButtonClicked);
	SoundEffectButton->OnClicked.AddUniqueDynamic(this, &USettingsWidget::ToggleSoundEffects);
	MusicButton->OnClicked.AddUniqueDynamic(this, &USettingsWidget::ToggleMusic);

	if (PrivacyButton)
	{
		PrivacyButton->OnClicked.AddUniqueDynamic(this, &USettingsWidget::PrivacyButtonClicked);
	}

	if (UAdsSubsystem* Ads = UAdsSubsystem::Get(this))
	{
		Ads->OnConsentFinished.AddUObject(this, &USettingsWidget::UpdatePrivacyButton);
	}
	UpdatePrivacyButton();
}

void USettingsWidget::NativeDestruct()
{
	if (UAdsSubsystem* Ads = UAdsSubsystem::Get(this))
	{
		Ads->OnConsentFinished.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void USettingsWidget::OnPopupOpening()
{
	UpdatePrivacyButton();
}

void USettingsWidget::UpdatePrivacyButton()
{
	if (!PrivacyButton)
	{
		return;
	}

	const UAdsSubsystem* Ads = UAdsSubsystem::Get(this);
	const bool bShow = Ads && Ads->IsPrivacyOptionsRequired();
	PrivacyButton->SetVisibility(bShow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void USettingsWidget::PrivacyButtonClicked()
{
	if (!IsOpen())
	{
		return;
	}

	if (UAdsSubsystem* Ads = UAdsSubsystem::Get(this))
	{
		Ads->ShowPrivacyOptions();
	}
}

void USettingsWidget::BackButtonClicked()
{
	Close();
}

void USettingsWidget::UpdateBordersVisuals(UBorder* MutedBorder, UBorder* UnmutedBorder, bool bMuted) const
{
	const float MutedAlpha = bMuted ? 1.0f : 0.0f;
	const float UnmutedAlpha = bMuted ? 0.0f : 1.0f;

	FLinearColor FinalMutedColor = MutedColor;
	FinalMutedColor.A = MutedAlpha;

	FLinearColor FinalUnmutedColor = UnmutedColor;
	FinalUnmutedColor.A = UnmutedAlpha;

	if (MutedBorder)
	{
		MutedBorder->SetBrushColor(FinalMutedColor);
		FSlateBrush Brush = MutedBorder->Background;
		Brush.OutlineSettings.Color = FLinearColor(0, 0, 0, MutedAlpha);
		MutedBorder->SetBrush(Brush);
	}

	if (UnmutedBorder)
	{
		UnmutedBorder->SetBrushColor(FinalUnmutedColor);
		FSlateBrush Brush = UnmutedBorder->Background;
		Brush.OutlineSettings.Color = FLinearColor(0, 0, 0, UnmutedAlpha);
		UnmutedBorder->SetBrush(Brush);
	}
}

void USettingsWidget::ToggleSoundEffects()
{
	USaveSubsystem* Save = USaveSubsystem::Get(this);
	if (!Save)
	{
		return;
	}

	FUserProgression UserProgression = Save->GetProgression();
	UserProgression.bIsSFXMuted = !UserProgression.bIsSFXMuted;

	UpdateBordersVisuals(MutedSFXBorder, UnmutedSFXBorder, UserProgression.bIsSFXMuted);
	ApplySFXVolume(UserProgression.bIsSFXMuted ? 0.0f : 1.0f);
	Save->Save(UserProgression);
}

void USettingsWidget::ToggleMusic()
{
	USaveSubsystem* Save = USaveSubsystem::Get(this);
	if (!Save)
	{
		return;
	}

	FUserProgression UserProgression = Save->GetProgression();
	UserProgression.bIsMusicMuted = !UserProgression.bIsMusicMuted;

	UpdateBordersVisuals(MutedMusicBorder, UnmutedMusicBorder, UserProgression.bIsMusicMuted);
	ApplyMusicMuted(UserProgression.bIsMusicMuted);
	Save->Save(UserProgression);
}

void USettingsWidget::ApplyMusicMuted(bool bMuted)
{
	UFlappyAudioSubsystem* Audio = UFlappyAudioSubsystem::Get(this);
	if (!Audio)
	{
		return;
	}

	if (bMuted)
	{
		Audio->StopMusic();
	}
	else
	{
		Audio->PlayMusic();
	}
}

void USettingsWidget::ApplySFXVolume(float Volume)
{
	if (!SFXSoundClass)
	{
		return;
	}

	if (SettingsSoundMix)
	{
		UGameplayStatics::SetSoundMixClassOverride(this, SettingsSoundMix, SFXSoundClass, Volume, 1.0f, 0.0f, true);
		if (!bSoundMixPushed)
		{
			UGameplayStatics::PushSoundMixModifier(this, SettingsSoundMix);
			bSoundMixPushed = true;
		}
		return;
	}

	SFXSoundClass->Properties.Volume = Volume;
}
