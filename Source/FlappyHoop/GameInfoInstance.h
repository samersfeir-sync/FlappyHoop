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
	virtual FUserProgression GetUserProgression() const override { return UserProgression; }

private:

	UPROPERTY()
	UMySaveGame* SaveGame = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "User Progression")
	FUserProgression UserProgression;

	virtual void Init() override;
};
