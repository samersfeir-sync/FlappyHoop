// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "GameModeInterface.h"
#include "BaseGameMode.generated.h"

class UGameWidget;

UCLASS()
class FLAPPYHOOP_API ABaseGameMode : public AGameMode, public IGameModeInterface
{
	GENERATED_BODY()

public:

	virtual FOnGameStarted& OnGameStartedDelegate() override { return OnGameStarted; }

	virtual FOnViewportFetched& OnViewportFetchedDelegate() { return OnViewportFetched; }

	virtual FOnGameReset& OnGameResetDelegate() { return OnGameReset; }

	virtual void ResetGame() override;

protected:

	virtual void BeginPlay() override;

	virtual FVector2D GetViewportSize() const override { return ViewportSize; }

private:

	UGameWidget* GameWidgetInstance = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameWidget> GameWidgetClass;

	FOnGameStarted OnGameStarted;

	FOnViewportFetched OnViewportFetched;

	FOnGameReset OnGameReset;

	void FetchViewportSize();

	FVector2D ViewportSize;

	UWorld* World = nullptr;
};
