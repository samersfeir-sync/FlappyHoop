// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Ground.generated.h"

/**
 * 
 */
UCLASS()
class FLAPPYHOOP_API AGround : public AActor
{
	GENERATED_BODY()
	
public:

	AGround();

private:

	class IGameModeInterface* GameModeInterface;

	virtual void BeginPlay() override;

	UFUNCTION()
	void OnHit(
		AActor* SelfActor,
		AActor* OtherActor,
		FVector NormalImpulse,
		const FHitResult& Hit);

	UPROPERTY(EditAnywhere, Category = "Components")
	class UBoxComponent* Ground;

	bool bAlreadyProcessedHit = false;

	void ResetGround();
};
