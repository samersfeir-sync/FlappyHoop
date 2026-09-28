// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/Common/PopupWidget.h"
#include "SettingsWidget.generated.h"

class UButton;
class UBorder;
class USoundClass;
class USoundMix;

UCLASS()
class FLAPPYHOOP_API USettingsWidget : public UPopupWidget
{
	GENERATED_BODY()

public:

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> UnmutedSFXBorder;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> MutedSFXBorder;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> UnmutedMusicBorder;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> MutedMusicBorder;

	void UpdateBordersVisuals(UBorder* MutedBorder, UBorder* UnmutedBorder, bool bMuted) const;

protected:

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void OnPopupOpening() override;

private:

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BackButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SoundEffectButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> MusicButton;

	/** Optional: opens Google's privacy options (shown only where Google requires it, e.g. the EEA/UK). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> PrivacyButton;

	UFUNCTION()
	void PrivacyButtonClicked();

	void UpdatePrivacyButton();

	UFUNCTION()
	void BackButtonClicked();

	UFUNCTION()
	void ToggleSoundEffects();

	UFUNCTION()
	void ToggleMusic();

	/** (Renamed from an overload of ToggleMusic - UFUNCTION overloads are ambiguous for delegate binding.) */
	void ApplyMusicMuted(bool bMuted);

	void ApplySFXVolume(float Volume);

	UPROPERTY(EditDefaultsOnly, Category = "Colors")
	FLinearColor MutedColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Colors")
	FLinearColor UnmutedColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	TObjectPtr<USoundClass> SFXSoundClass;

	/**
	 * Optional. If set, SFX volume is changed through this sound mix (the proper runtime way).
	 * If empty, falls back to editing the sound class volume directly (old behaviour; in PIE that edits the asset).
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	TObjectPtr<USoundMix> SettingsSoundMix;

	bool bSoundMixPushed = false;
};
