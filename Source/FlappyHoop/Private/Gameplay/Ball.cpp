// Fill out your copyright notice in the Description page of Project Settings.


#include "Gameplay/Ball.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Framework/BaseGameMode.h"
#include "Gameplay/ScreenEdges.h"

ABall::ABall()
{
	PrimaryActorTick.bCanEverTick = false;

	BallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BallMesh"));
	RootComponent = BallMesh;

	BallMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BallMesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	BallMesh->SetSimulatePhysics(false);
	BallMesh->SetEnableGravity(false);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->bAutoActivate = false;
	ProjectileMovement->InitialSpeed = 0.0f;
	ProjectileMovement->MaxSpeed = 3000.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = true;
	ProjectileMovement->Bounciness = 0.7f;
	ProjectileMovement->Friction = 0.1f;
	ProjectileMovement->ProjectileGravityScale = 2.0f;
	ProjectileMovement->bInitialVelocityInLocalSpace = false;
	ProjectileMovement->BounceVelocityStopSimulatingThreshold = 0.0f;

	SetActorHiddenInGame(true);
}

void ABall::BeginPlay()
{
	Super::BeginPlay();

	InitialBallLocation = GetActorLocation();
	InitialBallVelocity = ProjectileMovement->Velocity;
	InitialDirection = Direction;

	GameMode = ABaseGameMode::Get(this);

	if (GameMode)
	{
		GameMode->OnGameStarted.AddUObject(this, &ABall::ActivateBall);
		GameMode->OnGameReset.AddUObject(this, &ABall::ResetBall);
	}
}

void ABall::LaunchBall()
{
	if (!ProjectileMovement)
	{
		return;
	}

	ProjectileMovement->Velocity = FVector(HorizontalImpulse * Direction, 0.0f, VerticalImpulse);
	ProjectileMovement->Activate(true);
	UGameplayStatics::PlaySound2D(this, FlapSound);
}

FVector ABall::GetBallVelocity() const
{
	return ProjectileMovement ? ProjectileMovement->Velocity : FVector::ZeroVector;
}

void ABall::ChangeBallDirection()
{
	Direction *= -1;
	const bool bIsRight = (Direction == 1);

	if (RightEdge) { RightEdge->ActivateEdge(bIsRight); }
	if (LeftEdge)  { LeftEdge->ActivateEdge(!bIsRight); }
}

void ABall::ActivateBall()
{
	SetActorHiddenInGame(false);
	ProjectileMovement->Activate(true);
}

void ABall::ResetBall()
{
	SetActorLocation(InitialBallLocation, false, nullptr, ETeleportType::TeleportPhysics);
	ProjectileMovement->Velocity = InitialBallVelocity;
	SetActorHiddenInGame(true);
	ProjectileMovement->Deactivate();
	Direction = InitialDirection; // was hard-coded to -1, ignoring the editor value
}
