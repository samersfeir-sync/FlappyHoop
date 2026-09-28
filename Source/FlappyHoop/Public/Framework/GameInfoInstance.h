// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GameInfoInstance.generated.h"

class UMySaveGame;
class USoundBase;
class UBallCatalog;

/**
 * Thin game instance: holds the Blueprint-edited settings the subsystems read, and runs the startup sequence
 * (music + banner once the startup movie is done). The actual work lives in the subsystems:
 * USaveSubsystem, UEconomySubsystem, UStoreSubsystem, UAdsSubsystem, UFlappyAudioSubsystem.
 */
UCLASS()
class FLAPPYHOOP_API UGameInfoInstance : public UGameInstance
{
	GENERATED_BODY()

public:

	virtual void Init() override;
	virtual void OnStart() override;
	virtual void Shutdown() override;

	TSubclassOf<UMySaveGame> GetSaveGameClass() const { return SaveGameClass; }
	USoundBase* GetGameMusic() const { return GameMusic; }
	int32 GetMaxBillingReconnectAttempts() const { return MaxBillingReconnectAttempts; }
	const UBallCatalog* GetBallCatalog() const { return BallCatalog; }
	bool ShouldDebugConsentAsEEA() const { return bDebugConsentAsEEA; }

private:

	/** Save object class; its defaults are the starting progression (including the shop's ball list). */
	UPROPERTY(EditDefaultsOnly, Category = "Save")
	TSubclassOf<UMySaveGame> SaveGameClass;

	/** Every ball in the game (mesh, shop icon, price). */
	UPROPERTY(EditDefaultsOnly, Category = "Shop")
	TObjectPtr<UBallCatalog> BallCatalog;

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	TObjectPtr<USoundBase> GameMusic;

	UPROPERTY(EditDefaultsOnly, Category = "Billing")
	int32 MaxBillingReconnectAttempts = 6;

	/**
	 * Testing only (ignored in Shipping): pretend the device is in the EEA so Google's consent form shows.
	 * Your device must be listed in Project Settings > Admob Goodies > Test Device IDs.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Ads")
	bool bDebugConsentAsEEA = false;

	void HandleStartupMovieFinished();
	void StartMusicAndBanner();
	bool bStartupHandled = false;
};
