// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SettingsWidget.generated.h"

class UButton;
class UBorder;

UCLASS()
class FLAPPYHOOP_API USettingsWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:

	UPROPERTY(meta = (BindWidget))
	UBorder* UnmutedSFXBorder;

	UPROPERTY(meta = (BindWidget))
	UBorder* MutedSFXBorder;

	UPROPERTY(meta = (BindWidget))
	UBorder* UnmutedMusicBorder;

	UPROPERTY(meta = (BindWidget))
	UBorder* MutedMusicBorder;

	void UpdateBordersVisuals(UBorder* MutedBorder, UBorder* UnmutedBorder, bool bMuted) const;

private:

	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	UButton* BackButton;

	UPROPERTY(meta = (BindWidget))
	UButton* SoundEffectButton;

	UPROPERTY(meta = (BindWidget))
	UButton* MusicButton;

	UFUNCTION()
	void BackButtonClicked();

	class IGameInstanceInterface* GameInstanceInterface = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Colors")
	FLinearColor MutedColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Colors")
	FLinearColor UnmutedColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);

	UFUNCTION()
	void ToggleSoundEffects();

	UFUNCTION()
	void ToggleMusic();

	void SetSoundClassVolume(USoundClass* SoundClass, float Volume);

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	USoundClass* SFXSoundClass;

	void ToggleMusic(bool bMuted);
};
