// Fill out your copyright notice in the Description page of Project Settings.

#include "ScreenEdges.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "FunctionsLibrary.h"
#include "GameModeInterface.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraActor.h"

// Sets default values
AScreenEdges::AScreenEdges()
{
	PrimaryActorTick.bCanEverTick = false;
	EdgeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EdgeMesh"));
	RootComponent = EdgeMesh;
	EdgeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EdgeMesh->SetHiddenInGame(true);
}

// Called when the game starts or when spawned
void AScreenEdges::BeginPlay()
{
	Super::BeginPlay();

	GameModeInterface = UFunctionsLibrary::GetGameModeInterface(this);

	if (GameModeInterface)
	{
		GameModeInterface->OnViewportFetchedDelegate().AddUObject(this, &AScreenEdges::SetEdgeLocation);
	}
}

// Called every frame
void AScreenEdges::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AScreenEdges::GetCameraFrustumEdges(UCameraComponent* Camera, float Distance,
	FVector& LeftCenter, FVector& RightCenter)
{
	FMinimalViewInfo CameraView;
	Camera->GetCameraView(0.0f, CameraView);

	float AspectRatio = CameraView.AspectRatio;
	float FoV_Y = CameraView.FOV;

	float HalfHeight = Distance * FMath::Tan(FMath::DegreesToRadians(FoV_Y) * 0.5f);
	float HalfWidth = HalfHeight * AspectRatio;

	FVector CameraForward = Camera->GetForwardVector();
	FVector CameraRight = Camera->GetRightVector();
	FVector CameraLocation = CameraView.Location;

	FVector CenterAtDistance = CameraLocation + (CameraForward * Distance);
	LeftCenter = CenterAtDistance - (CameraRight * HalfWidth);
	RightCenter = CenterAtDistance + (CameraRight * HalfWidth);
}

void AScreenEdges::SetEdgeLocation()
{
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();

	AActor* ViewTarget = PlayerController->GetViewTarget();
	ACameraActor* Camera = Cast<ACameraActor>(ViewTarget);

	UCameraComponent* CameraComponent = Camera->GetCameraComponent();

	FVector LeftCenter;
	FVector RightCenter;

	FVector2D ViewportSize = GameModeInterface->GetViewportSize();

	float Distance = ViewportSize.X * 170.0f / 720.0f;
	GetCameraFrustumEdges(CameraComponent, Distance, LeftCenter, RightCenter);

	FVector TargetLocation = RightEdge ? RightCenter : LeftCenter;
	TargetLocation.Y = -500.f; //align with ball Y position
	TargetLocation.Z = 0.0f;
	SetActorLocation(TargetLocation);
}