// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Ground.generated.h"

class ABaseGameMode;
class UBoxComponent;

/** Floor. Tells the game mode whenever the ball lands; the game mode decides what that means. */
UCLASS()
class FLAPPYHOOP_API AGround : public AActor
{
	GENERATED_BODY()

public:

	AGround();

protected:

	virtual void BeginPlay() override;

private:

	UPROPERTY(Transient)
	TObjectPtr<ABaseGameMode> GameMode = nullptr;

	UFUNCTION()
	void OnHit(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit);

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UBoxComponent> Ground;
};
