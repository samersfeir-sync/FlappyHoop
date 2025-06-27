// Fill out your copyright notice in the Description page of Project Settings.

#include "ScreenEdges.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "FunctionsLibrary.h"
#include "GameModeInterface.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraActor.h"
#include "CameraMain.h"
#include "Ball.h"
#include "Hoop.h"

// Sets default values
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
	Arrows.Add(UpArrow);

	MiddleArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("MiddleArrow"));
	MiddleArrow->SetupAttachment(EdgeMesh);
	Arrows.Add(MiddleArrow);

	DownArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("DownArrow"));
	DownArrow->SetupAttachment(EdgeMesh);
	Arrows.Add(DownArrow);

	SetActorHiddenInGame(true);
}

// Called when the game starts or when spawned
void AScreenEdges::BeginPlay()
{
	Super::BeginPlay();

	EdgeMesh->OnComponentBeginOverlap.AddDynamic(this, &AScreenEdges::OnEdgeMeshBeginOverlap);

	GameModeInterface = UFunctionsLibrary::GetGameModeInterface(this);

	if (GameModeInterface)
	{
		GameModeInterface->OnGameStartedDelegate().AddUObject(this, &AScreenEdges::GameStarted);
		GameModeInterface->OnGameResetDelegate().AddUObject(this, &AScreenEdges::ResetEdge);
	}

	AActor* ChildActor = HoopChild->GetChildActor();

	HoopChildActor = Cast<AHoop>(ChildActor);

	if (HoopChildActor)
	{
		HoopChildActor->SetRingRotation(bRightEdge);
	}
}

// Called every frame
void AScreenEdges::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AScreenEdges::ActivateEdge(bool bActivate)
{
	SetActorHiddenInGame(!bActivate);
	SetActorEnableCollision(bActivate);

	AActor* ChildActor = HoopChild->GetChildActor();

	if (ChildActor)
	{
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
}

void AScreenEdges::GameStarted()
{
	ActivateEdge(!bRightEdge);
}

void AScreenEdges::ResetEdge()
{
	SetActorHiddenInGame(true);
}

void AScreenEdges::OnEdgeMeshBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	FVector BallLocation = Ball->GetActorLocation();
	BallLocation.X = -BallLocation.X;
	Ball->SetActorLocation(BallLocation);
}
