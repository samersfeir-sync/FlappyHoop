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

	HoopChild = CreateDefaultSubobject<UChildActorComponent>(TEXT("HoopChild"));
	HoopChild->SetupAttachment(EdgeMesh);

	UpArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("UpArrow"));
	UpArrow->SetupAttachment(EdgeMesh);
	Arrows.Add(UpArrow);

	MiddleArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("MiddleArrow"));
	MiddleArrow->SetupAttachment(EdgeMesh);
	Arrows.Add(MiddleArrow);

	DownArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("DownArrow"));
	DownArrow->SetupAttachment(EdgeMesh);
	Arrows.Add(DownArrow);
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

	ActivateEdge(!RightEdge);
}

// Called every frame
void AScreenEdges::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AScreenEdges::ActivateEdge(bool bActivate)
{
	SetActorHiddenInGame(!bActivate);
	AActor* ChildActor = HoopChild->GetChildActor();
	ChildActor->SetActorEnableCollision(bActivate);

	if (bActivate)
	{
		int32 Index = FMath::RandRange(0, Arrows.Num() - 1);
		UArrowComponent* SelectedArrow = Arrows[Index];

		if (SelectedArrow)
		{
			FVector ArrowLocation = SelectedArrow->GetComponentLocation();
			FVector CurrentLocation = ChildActor->GetActorLocation();
			FVector NewLocation = FVector(CurrentLocation.X, CurrentLocation.Y, ArrowLocation.Z);
			ChildActor->SetActorLocation(NewLocation);
		}
	}
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