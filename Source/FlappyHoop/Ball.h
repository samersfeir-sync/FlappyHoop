// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Ball.generated.h"

class IGameModeInterface;

UCLASS()
class FLAPPYHOOP_API ABall : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABall();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void LaunchBall();

private:

	UPROPERTY(EditDefaultsOnly, Category = "Components")
	class UStaticMeshComponent* BallMesh;

	UPROPERTY(EditAnywhere, Category = "Components")
	class UProjectileMovementComponent* ProjectileMovement;

	UPROPERTY(EditAnywhere, Category = "Parameters")
	float HorizontalImpulse = 500.0f;

	UPROPERTY(EditAnywhere, Category = "Parameters")
	float VerticalImpulse = 800.0f;

	int Direction = -1;

	APlayerController* PlayerController = nullptr;
	APlayerCameraManager* CameraManager = nullptr;

	bool HandleScreenWrap(FVector& Location);

	float YLocation = -500.0f;
	FVector InitialBallLocation;
	FVector InitialBallVelocity;

	IGameModeInterface* GameModeInterface = nullptr;

	void ActivateBall();
	void ResetBall();
};
