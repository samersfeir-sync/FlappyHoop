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

private:

	UPROPERTY()
	UMySaveGame* SaveGame = nullptr;

	FUserProgression UserProgression;

	virtual void Init() override;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UMySaveGame> SaveGameClass;

	void InitializeADUnits();

	UPROPERTY(BlueprintReadOnly, Category = "Advertisement", meta = (AllowPrivateAccess = "true"))
	FString BannerADUnitID;

	UPROPERTY(BlueprintReadOnly, Category = "Advertisement", meta = (AllowPrivateAccess = "true"))
	FString InterstitialADUnitID;

	UPROPERTY(BlueprintReadOnly, Category = "Advertisement", meta = (AllowPrivateAccess = "true"))
	FString RewardedADUnitID;
};
