// Fill out your copyright notice in the Description page of Project Settings.


#include "Ball.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "FunctionsLibrary.h"
#include "GameModeInterface.h"

// Sets default values
ABall::ABall()
{
	PrimaryActorTick.bCanEverTick = true;

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

    PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (PlayerController)
    {
        CameraManager = PlayerController->PlayerCameraManager;
    }

    YLocation = GetActorLocation().Y;

    GameModeInterface = UFunctionsLibrary::GetGameModeInterface(this);

    if (GameModeInterface)
    {
        GameModeInterface->OnGameStartedDelegate().AddUObject(this, &ABall::ActivateBall);
    }
}

// Called every frame
void ABall::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    FVector Location = GetActorLocation();
    Location.Y = YLocation;

    if (HandleScreenWrap(Location))
    {
        SetActorLocation(Location);
    }
}

void ABall::LaunchBall()
{
	if (ProjectileMovement)
	{   
		FVector LaunchVelocity = FVector(HorizontalImpulse * Direction, 0.0f, VerticalImpulse);
		ProjectileMovement->Velocity = LaunchVelocity;
        ProjectileMovement->Activate(true);
	}
}

bool ABall::HandleScreenWrap(FVector& Location)
{
    if (!PlayerController || !CameraManager) return false;

    FVector2D ScreenPosition;
    PlayerController->ProjectWorldLocationToScreen(Location, ScreenPosition);

    FVector2D ViewportSize;
    GEngine->GameViewport->GetViewportSize(ViewportSize);

    const float Buffer = 10.0f;
    bool bTeleported = false;

    if (ScreenPosition.X < -Buffer)
    {
        ScreenPosition.X = ViewportSize.X + Buffer;
        bTeleported = true;
    }
    else if (ScreenPosition.X > ViewportSize.X + Buffer)
    {
        ScreenPosition.X = -Buffer;
        bTeleported = true;
    }

    if (bTeleported)
    {
        FVector CameraLocation = CameraManager->GetCameraLocation();
        FVector CameraForward = CameraManager->GetActorForwardVector();
        float Distance = FVector::DotProduct(Location - CameraLocation, CameraForward);

        FVector WorldLocation, WorldDirection;
        PlayerController->DeprojectScreenPositionToWorld(ScreenPosition.X, ScreenPosition.Y, WorldLocation, WorldDirection);

        FVector NewWorldPos = CameraLocation + WorldDirection.GetSafeNormal() * Distance;
        NewWorldPos.Y = YLocation;
        Location = NewWorldPos;
    }

    return bTeleported;
}

void ABall::ActivateBall()
{
    SetActorHiddenInGame(false);
    ProjectileMovement->Activate(true);
}
