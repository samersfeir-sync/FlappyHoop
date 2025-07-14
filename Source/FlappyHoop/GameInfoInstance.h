// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GameInstanceInterface.h"
#include "GameInfoInstance.generated.h"

class UMySaveGame;

UCLASS()
class FLAPPYHOOP_API UGameInfoInstance : public UGameInstance, public IGameInstanceInterface
{
	GENERATED_BODY()

public:

	virtual void SaveUserProgression(FUserProgression& NewUserProgression) override;
	virtual void LoadUserProgression() override;
	virtual FUserProgression& GetUserProgression() override;
	virtual FString GetBannerAdUnitID() const override { return BannerADUnitID; }
	virtual FString GetInterstitialAdUnitID() const override { return InterstitialADUnitID; }
	virtual FString GetRewardedAdUnitID() const override { return RewardedADUnitID; }
	virtual const TScriptInterface<IAGBannerAdInterface> GetBannerAdInterface() const { return BannerAdInterface; }

private:

	UPROPERTY()
	UMySaveGame* SaveGame = nullptr;

	FUserProgression UserProgression;

	virtual void Init() override;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UMySaveGame> SaveGameClass;

	void InitializeADUnits();

	FString BannerADUnitID;
	FString InterstitialADUnitID;

	FString RewardedADUnitID;

	void OnMoviePlaybackFinished();

	TScriptInterface<IAGBannerAdInterface> BannerAdInterface;

	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	USoundBase* GameMusic;

	virtual void PlayBackgroundMusic() override;

	virtual void StopBackgroundMusic() override;

	UPROPERTY()
	UAudioComponent* MusicAudioComponent;
};
