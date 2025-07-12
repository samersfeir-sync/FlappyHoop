// Fill out your copyright notice in the Description page of Project Settings.


#include "Ball.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "FunctionsLibrary.h"
#include "GameModeInterface.h"
#include "ScreenEdges.h"

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

// Called when the game starts or when spawned
void ABall::BeginPlay()
{
	Super::BeginPlay();

    InitialBallLocation = GetActorLocation();
    YLocation = InitialBallLocation.Y;
    InitialBallVelocity = ProjectileMovement->Velocity;

    GameModeInterface = UFunctionsLibrary::GetGameModeInterface(this);

    if (GameModeInterface)
    {
        GameModeInterface->OnGameStartedDelegate().AddUObject(this, &ABall::ActivateBall);
        GameModeInterface->OnGameResetDelegate().AddUObject(this, &ABall::ResetBall);
    }
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
		FVector LaunchVelocity = FVector(HorizontalImpulse * Direction, 0.0f, VerticalImpulse);
		ProjectileMovement->Velocity = LaunchVelocity;
        ProjectileMovement->Activate(true);
        UGameplayStatics::PlaySound2D(this, FlapSound);
	}
}

FVector ABall::GetBallVelocity() const
{
    return  ProjectileMovement->Velocity;
}

void ABall::ChangeBallDirection()
{
    Direction *= -1;
    bool bIsRight = (Direction == 1);
    RightEdge->ActivateEdge(bIsRight);
    LeftEdge->ActivateEdge(!bIsRight);
}

void ABall::ActivateBall()
{
    SetActorHiddenInGame(false);
    ProjectileMovement->Activate(true);
}

void ABall::ResetBall()
{
    SetActorLocation(InitialBallLocation);
    ProjectileMovement->Velocity = InitialBallVelocity;
    SetActorHiddenInGame(true);
    ProjectileMovement->Deactivate();
    Direction = -1;
}
