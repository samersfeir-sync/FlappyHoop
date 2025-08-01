// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Coins.generated.h"

UCLASS()
class FLAPPYHOOP_API ACoins : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACoins();

	void ActivateCoin(bool bActivate);

	bool IsCoinActive() const { return bActive; }

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	USoundBase* PickUpSound;

	UFUNCTION()
	virtual void OnCoinBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	class IGameModeInterface* GameModeInterface;

	UPROPERTY(EditDefaultsOnly, Category = "Components")
	UStaticMeshComponent* CoinMesh;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:

	bool bActive = false;

	UPROPERTY(EditInstanceOnly)
	TArray<class ATargetPoint*> CoinLocations;

	void ResetCoin();
};
