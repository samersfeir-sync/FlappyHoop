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

	AHoop* Hoop = Cast<AHoop>(ChildActor);
	if (!bActivate)
	{
		if (Hoop) { Hoop->StopDrift(); }
		return;
	}

	if (!UpArrow || !MiddleArrow || !DownArrow)
	{
		return;
	}

	const double UpZ = UpArrow->GetComponentLocation().Z;
	const double MiddleZ = MiddleArrow->GetComponentLocation().Z;
	const double DownZ = DownArrow->GetComponentLocation().Z;
	const double TopZ = FMath::Max(UpZ, DownZ);
	const double BottomZ = FMath::Min(UpZ, DownZ);

	double NewZ = MiddleZ;
	if (!GameMode || GameMode->UsesAllHoopHeights())
	{
		// Hard (and the old behaviour): one of the three arrows.
		if (const UArrowComponent* SelectedArrow = Arrows.Num() > 0 ? Arrows[FMath::RandRange(0, Arrows.Num() - 1)].Get() : nullptr)
		{
			NewZ = SelectedArrow->GetComponentLocation().Z;
		}
	}
	else
	{
		// Easy: a random height in a band around the Middle arrow. Band 1 reaches the Up/Down arrows.
		const double Band = FMath::Clamp(GameMode->GetHoopHeightBand(), 0.0f, 1.0f);
		const double T = FMath::FRandRange(-Band, Band);
		NewZ = T >= 0.0 ? MiddleZ + T * (TopZ - MiddleZ) : MiddleZ + T * (MiddleZ - BottomZ);
	}

	FVector NewLocation = ChildActor->GetActorLocation();
	NewLocation.Z = NewZ;
	ChildActor->SetActorLocation(NewLocation);

	if (Hoop)
	{
		Hoop->StartDrift(NewZ, BottomZ, TopZ);
	}
}

void AScreenEdges::ResetEdge()
{
	SetActorHiddenInGame(true);

	if (AHoop* Hoop = Cast<AHoop>(HoopChild->GetChildActor()))
	{
		Hoop->StopDrift();
	}
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
