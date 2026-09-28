// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Gameplay/BallInterface.h"
#include "Ball.generated.h"

class ABaseGameMode;
class AScreenEdges;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class USoundBase;

UCLASS()
class FLAPPYHOOP_API ABall : public AActor, public IBallInterface
{
	GENERATED_BODY()

public:

	ABall();

	void LaunchBall();

	virtual FVector GetBallVelocity() const override;
	virtual void ChangeBallDirection() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BallMesh;

protected:

	virtual void BeginPlay() override;

private:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(EditAnywhere, Category = "Parameters")
	float HorizontalImpulse = 500.0f;

	UPROPERTY(EditAnywhere, Category = "Parameters")
	float VerticalImpulse = 800.0f;

	/** -1 = moving left, +1 = moving right. */
	UPROPERTY(EditAnywhere, Category = "Parameters")
	int32 Direction = -1;

	int32 InitialDirection = -1;
	FVector InitialBallLocation = FVector::ZeroVector;
	FVector InitialBallVelocity = FVector::ZeroVector;

	UPROPERTY(Transient)
	TObjectPtr<ABaseGameMode> GameMode = nullptr;

	void ActivateBall();
	void ResetBall();

	UPROPERTY(EditInstanceOnly, Category = "Parameters")
	TObjectPtr<AScreenEdges> LeftEdge;

	UPROPERTY(EditInstanceOnly, Category = "Parameters")
	TObjectPtr<AScreenEdges> RightEdge;

	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	TObjectPtr<USoundBase> FlapSound;
};
