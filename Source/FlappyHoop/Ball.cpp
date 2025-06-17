// Fill out your copyright notice in the Description page of Project Settings.


#include "Ball.h"
#include "GameFramework/ProjectileMovementComponent.h"

// Sets default values
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
	ProjectileMovement->SetUpdatedComponent(BallMesh);
	ProjectileMovement->InitialSpeed = 0.0f;
	ProjectileMovement->MaxSpeed = 3000.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = true;
	ProjectileMovement->Bounciness = 0.7f;
	ProjectileMovement->Friction = 0.1f;
	ProjectileMovement->ProjectileGravityScale = 2.0f;
	ProjectileMovement->bInitialVelocityInLocalSpace = false;
}

// Called when the game starts or when spawned
void ABall::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ABall::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ABall::LaunchBall()
{
	if (ProjectileMovement)
	{
		FVector LaunchVelocity = FVector(HorizontalImpulse, 0.0f, VerticalImpulse);
		ProjectileMovement->Velocity = LaunchVelocity;
	}
}
