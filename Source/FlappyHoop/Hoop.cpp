// Fill out your copyright notice in the Description page of Project Settings.


#include "Hoop.h"
#include "BallInterface.h"
#include "GameModeInterface.h"
#include "FunctionsLibrary.h"

// Sets default values
AHoop::AHoop()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	HoopMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HoopMesh"));
	RootComponent = HoopMesh;

	ScoreCylinder = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ScoreCylinder"));
	ScoreCylinder->SetupAttachment(HoopMesh);
	ScoreCylinder->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ScoreCylinder->SetCollisionResponseToAllChannels(ECR_Overlap);
	ScoreCylinder->SetGenerateOverlapEvents(true);
}

// Called when the game starts or when spawned
void AHoop::BeginPlay()
{
	Super::BeginPlay();
	ScoreCylinder->OnComponentBeginOverlap.AddDynamic(this, &AHoop::OnScoreCylinderBeginOverlap);

	GameModeInterface = UFunctionsLibrary::GetGameModeInterface(this);

}

// Called every frame
void AHoop::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AHoop::OnScoreCylinderBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor->GetClass()->ImplementsInterface(UBallInterface::StaticClass()))
	{
		if (IBallInterface* BallInterface = Cast<IBallInterface>(OtherActor))
		{
			FVector BallVelocity = BallInterface->GetBallVelocity();
			BallInterface->ChangeBallDirection();
			if (GameModeInterface)
			{
				GameModeInterface->SetNewGameTime();
				GameModeInterface->UpdateScore();
				GameModeInterface->OnPointScoredDelegate().Broadcast();
			}
		}
	}
}
