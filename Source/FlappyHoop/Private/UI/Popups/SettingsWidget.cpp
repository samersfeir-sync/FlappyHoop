// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Popups/SettingsWidget.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"
#include "Systems/SaveSubsystem.h"
#include "Systems/FlappyAudioSubsystem.h"
#include "Systems/AdsSubsystem.h"
#include "Systems/StoreSubsystem.h"
#include "Systems/CloudSaveSubsystem.h"
#include "Components/TextBlock.h"
#include "Core/NativeDialog.h"

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

	if (RestorePurchasesButton)
	{
		// Always visible (so the layout can be checked anywhere); disabled where there is nothing to restore.
		RestorePurchasesButton->SetVisibility(ESlateVisibility::Visible);
		RestorePurchasesButton->SetIsEnabled(CanRestore());
		RestorePurchasesButton->OnClicked.AddUniqueDynamic(this, &USettingsWidget::RestorePurchasesClicked);

		if (UStoreSubsystem* Store = UStoreSubsystem::Get(this))
		{
			Store->OnPurchasesRestored.AddUObject(this, &USettingsWidget::HandlePurchasesRestored);
		}
	}

	if (UCloudSaveSubsystem* CloudSave = UCloudSaveSubsystem::Get(this))
	{
		CloudSave->OnSyncFinished.AddUObject(this, &USettingsWidget::HandleCloudSyncFinished);
	}

	SetRestoreStatus(FText::GetEmpty());
}

void USettingsWidget::NativeDestruct()
{
	if (UAdsSubsystem* Ads = UAdsSubsystem::Get(this))
	{
		Ads->OnConsentFinished.RemoveAll(this);
	}

	if (UStoreSubsystem* Store = UStoreSubsystem::Get(this))
	{
		Store->OnPurchasesRestored.RemoveAll(this);
	}

	if (UCloudSaveSubsystem* CloudSave = UCloudSaveSubsystem::Get(this))
	{
		CloudSave->OnSyncFinished.RemoveAll(this);
	}

	Super::NativeDestruct();
}

bool USettingsWidget::CanRestore() const
{
	const UCloudSaveSubsystem* CloudSave = UCloudSaveSubsystem::Get(this);
	return UStoreSubsystem::ShouldShowRestoreButton() || (CloudSave && CloudSave->IsSupported());
}

void USettingsWidget::RestorePurchasesClicked()
{
	if (!IsOpen() || !RestorePurchasesButton || !CanRestore() || bWaitingForStoreRestore || bWaitingForCloudRestore)
	{
		return;
	}

	UStoreSubsystem* Store = UStoreSubsystem::Get(this);
	UCloudSaveSubsystem* CloudSave = UCloudSaveSubsystem::Get(this);
	const bool bCloud = CloudSave && CloudSave->IsSupported();
	if (!Store && !bCloud)
	{
		return;
	}

	// Set both flags before starting either: the store can answer synchronously (Android).
	bWaitingForStoreRestore = Store != nullptr;
	bWaitingForCloudRestore = bCloud;
	CloudRestoreResult = static_cast<uint8>(EFlappyCloudSyncResult::Unsupported);

	RestorePurchasesButton->SetIsEnabled(false); // re-enabled when both answers are in
	SetRestoreStatus(RestoringText);

	if (bCloud)
	{
		CloudSave->RestoreNow(); // gems, coins, balls, high scores (signs in to Google Play Games if needed)
	}
	if (Store)
	{
		Store->RestorePurchases(); // "remove ads"
	}
}

void USettingsWidget::HandlePurchasesRestored(bool bAdsRemoved)
{
	if (!bWaitingForStoreRestore)
	{
		return;
	}
	bWaitingForStoreRestore = false;
	FinishRestoreIfDone();
}

void USettingsWidget::HandleCloudSyncFinished(EFlappyCloudSyncResult Result)
{
	if (!bWaitingForCloudRestore)
	{
		return;
	}
	bWaitingForCloudRestore = false;
	CloudRestoreResult = static_cast<uint8>(Result);
	FinishRestoreIfDone();
}

void USettingsWidget::FinishRestoreIfDone()
{
	if (bWaitingForStoreRestore || bWaitingForCloudRestore)
	{
		return;
	}

	if (RestorePurchasesButton)
	{
		RestorePurchasesButton->SetIsEnabled(CanRestore());
	}

	FText Result;
	switch (static_cast<EFlappyCloudSyncResult>(CloudRestoreResult))
	{
	case EFlappyCloudSyncResult::Synced:          Result = RestoredText; break;
	case EFlappyCloudSyncResult::NotSignedIn:     Result = NotSignedInText; break;
	case EFlappyCloudSyncResult::NoICloudAccount: Result = NoICloudText; break;
	case EFlappyCloudSyncResult::Failed:          Result = RestoreFailedText; break;
	default:                                      Result = PurchasesRestoredText; break;
	}

	// Phones: the system popup (AlertDialog / UIAlertController). Editor/desktop: the optional status text.
	if (FlappyNativeDialog::IsSupported())
	{
		SetRestoreStatus(FText::GetEmpty());
		FlappyNativeDialog::Show(RestoreDialogTitle, Result, RestoreDialogButton);
	}
	else
	{
		SetRestoreStatus(Result);
	}
}

void USettingsWidget::SetRestoreStatus(const FText& Text)
{
	if (RestoreStatusText)
	{
		RestoreStatusText->SetText(Text);
		RestoreStatusText->SetVisibility(Text.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

void USettingsWidget::OnPopupOpening()
{
	UpdatePrivacyButton();

	// Don't show an old restore result the next time Settings opens.
	if (!bWaitingForStoreRestore && !bWaitingForCloudRestore)
	{
		SetRestoreStatus(FText::GetEmpty());
	}
}

void USettingsWidget::UpdatePrivacyButton()
{
	if (!PrivacyButton || !PrivacySizeBox)
	{
		return;
	}

	const UAdsSubsystem* Ads = UAdsSubsystem::Get(this);
	const bool bShow = Ads && Ads->IsPrivacyOptionsRequired();
	PrivacySizeBox->SetVisibility(bShow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
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
