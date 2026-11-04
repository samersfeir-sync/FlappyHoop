// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/Common/PopupWidget.h"
#include "SettingsWidget.generated.h"

class UButton;
class UBorder;
class UTextBlock;
class USizeBox;
enum class EFlappyCloudSyncResult : uint8;
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

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> PrivacySizeBox;

	UFUNCTION()
	void PrivacyButtonClicked();

	void UpdatePrivacyButton();

	/**
	 * Optional: "Restore Purchases". Restores "remove ads" from the store AND gems, coins, balls and high scores
	 * from the cloud save. Always visible; enabled on iOS (App Store Review requires it) and wherever cloud save
	 * works (Android). Disabled in the editor/desktop unless bSimulateCloudSave is on.
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> RestorePurchasesButton;

	/**
	 * Optional: one line under the button. Shows "Restoring..." while it runs; the result is shown in the phone's own
	 * popup on Android/iOS, and in this text in the editor/desktop. Cleared when Settings opens again.
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RestoreStatusText;

	UFUNCTION()
	void RestorePurchasesClicked();

	void HandlePurchasesRestored(bool bAdsRemoved);
	void HandleCloudSyncFinished(EFlappyCloudSyncResult Result);
	void FinishRestoreIfDone();

	/** True where pressing Restore does something (iOS store restore, or a cloud save backend). */
	bool CanRestore() const;
	void SetRestoreStatus(const FText& Text);

	bool bWaitingForStoreRestore = false;
	bool bWaitingForCloudRestore = false;
	uint8 CloudRestoreResult = 0; // EFlappyCloudSyncResult

	/** Title and button of the phone popup that reports the restore result. */
	UPROPERTY(EditDefaultsOnly, Category = "Restore")
	FText RestoreDialogTitle = NSLOCTEXT("FlappyHoop", "RestoreDialogTitle", "Restore Purchases");

	UPROPERTY(EditDefaultsOnly, Category = "Restore")
	FText RestoreDialogButton = NSLOCTEXT("FlappyHoop", "RestoreDialogButton", "OK");

	UPROPERTY(EditDefaultsOnly, Category = "Restore")
	FText RestoringText = NSLOCTEXT("FlappyHoop", "Restoring", "Restoring...");

	UPROPERTY(EditDefaultsOnly, Category = "Restore")
	FText RestoredText = NSLOCTEXT("FlappyHoop", "Restored", "Purchases and progress restored");

	/** Shown when cloud save isn't available here (only the store restore ran). */
	UPROPERTY(EditDefaultsOnly, Category = "Restore")
	FText PurchasesRestoredText = NSLOCTEXT("FlappyHoop", "PurchasesRestored", "Purchases restored");

	UPROPERTY(EditDefaultsOnly, Category = "Restore")
	FText NotSignedInText = NSLOCTEXT("FlappyHoop", "RestoreNotSignedIn", "Sign in to Google Play Games to restore your progress");

	UPROPERTY(EditDefaultsOnly, Category = "Restore")
	FText NoICloudText = NSLOCTEXT("FlappyHoop", "RestoreNoICloud", "Sign in to iCloud to back up and restore your progress");

	UPROPERTY(EditDefaultsOnly, Category = "Restore")
	FText RestoreFailedText = NSLOCTEXT("FlappyHoop", "RestoreFailed", "Couldn't reach the cloud. Please try again later.");

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
