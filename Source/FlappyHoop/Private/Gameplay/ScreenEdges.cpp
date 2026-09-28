// Fill out your copyright notice in the Description page of Project Settings.

#include "Gameplay/ScreenEdges.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/ArrowComponent.h"
#include "Framework/BaseGameMode.h"
#include "Gameplay/Ball.h"
#include "Gameplay/Hoop.h"

AScreenEdges::AScreenEdges()
{
	PrimaryActorTick.bCanEverTick = false;

	EdgeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EdgeMesh"));
	RootComponent = EdgeMesh;
	EdgeMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	EdgeMesh->SetCollisionResponseToAllChannels(ECR_Overlap);
	EdgeMesh->SetHiddenInGame(true);

	HoopChild = CreateDefaultSubobject<UChildActorComponent>(TEXT("HoopChild"));
	HoopChild->SetupAttachment(EdgeMesh);

	UpArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("UpArrow"));
	UpArrow->SetupAttachment(EdgeMesh);

	MiddleArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("MiddleArrow"));
	MiddleArrow->SetupAttachment(EdgeMesh);

	DownArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("DownArrow"));
	DownArrow->SetupAttachment(EdgeMesh);

	SetActorHiddenInGame(true);
}

void AScreenEdges::BeginPlay()
{
	Super::BeginPlay();

	// Built at runtime rather than in the constructor so the list isn't baked into the CDO/instances.
	Arrows = { UpArrow, MiddleArrow, DownArrow };

	EdgeMesh->OnComponentBeginOverlap.AddUniqueDynamic(this, &AScreenEdges::OnEdgeMeshBeginOverlap);

	GameMode = ABaseGameMode::Get(this);
	if (GameMode)
	{
		GameMode->OnGameStarted.AddUObject(this, &AScreenEdges::GameStarted);
		GameMode->OnGameReset.AddUObject(this, &AScreenEdges::ResetEdge);
	}

	if (AHoop* Hoop = Cast<AHoop>(HoopChild->GetChildActor()))
	{
		Hoop->SetRingRotation(bRightEdge);
	}
}

void AScreenEdges::ActivateEdge(bool bActivate)
{
	SetActorHiddenInGame(!bActivate);
	SetActorEnableCollision(bActivate);

	AActor* ChildActor = HoopChild->GetChildActor();
	if (!ChildActor)
	{
		return;
	}

	ChildActor->SetActorEnableCollision(bActivate);

	if (bActivate && Arrows.Num() > 0)
	{
		if (const UArrowComponent* SelectedArrow = Arrows[FMath::RandRange(0, Arrows.Num() - 1)])
		{
			FVector NewLocation = ChildActor->GetActorLocation();
			NewLocation.Z = SelectedArrow->GetComponentLocation().Z;
			ChildActor->SetActorLocation(NewLocation);
		}
	}
}

void AScreenEdges::ResetEdge()
{
	SetActorHiddenInGame(true);
}

void AScreenEdges::OnEdgeMeshBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Only wrap the ball. Previously any overlap (coins, the hoop's own trigger meshes...) teleported the ball.
	AActor* Target = Ball ? static_cast<AActor*>(Ball.Get()) : nullptr;
	if (!Target && OtherActor && OtherActor->IsA<ABall>())
	{
		Target = OtherActor; // Ball reference not set on this edge instance - fall back to whatever ball overlapped.
	}

	if (!Target || OtherActor != Target)
	{
		return;
	}

	FVector BallLocation = Target->GetActorLocation();
	BallLocation.X = -BallLocation.X;
	Target->SetActorLocation(BallLocation, false, nullptr, ETeleportType::TeleportPhysics);
}
