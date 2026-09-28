// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Coins.generated.h"

class ABaseGameMode;
class ATargetPoint;
class UStaticMeshComponent;
class USoundBase;

UCLASS()
class FLAPPYHOOP_API ACoins : public AActor
{
	GENERATED_BODY()

public:

	ACoins();

	void ActivateCoin(bool bActivate);

	bool IsCoinActive() const { return bActive; }

protected:

	virtual void BeginPlay() override;

	/** Called when the ball picks this collectible up. Coins add a coin; AGems overrides to add a gem. */
	virtual void HandleCollected();

	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	TObjectPtr<USoundBase> PickUpSound;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> CoinMesh;

	UPROPERTY(Transient)
	TObjectPtr<ABaseGameMode> GameMode = nullptr;

private:

	UFUNCTION()
	void OnCoinBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	bool bActive = false;

	UPROPERTY(EditInstanceOnly, Category = "Spawning")
	TArray<TObjectPtr<ATargetPoint>> CoinLocations;

	void ResetCoin() { ActivateCoin(false); }
};
