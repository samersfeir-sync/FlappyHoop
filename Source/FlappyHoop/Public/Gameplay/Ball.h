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
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	/** Only ticks during a run: applies the difficulty's fall-speed cap. */
	virtual void Tick(float DeltaSeconds) override;

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

	/** Reads gravity / tap strength / fall cap for the current difficulty level. */
	void ApplyDifficulty(int32 Level);

	/** BP_Ball's own values, so the difficulty multipliers never compound. */
	float BaseGravityScale = 2.0f;
	float BaseVerticalImpulse = 800.0f;

	float CurrentVerticalImpulse = 800.0f;
	/** cm/s, 0 = no cap. */
	float MaxFallSpeed = 0.0f;

	UPROPERTY(EditInstanceOnly, Category = "Parameters")
	TObjectPtr<AScreenEdges> LeftEdge;

	UPROPERTY(EditInstanceOnly, Category = "Parameters")
	TObjectPtr<AScreenEdges> RightEdge;

	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	TObjectPtr<USoundBase> FlapSound;
};
