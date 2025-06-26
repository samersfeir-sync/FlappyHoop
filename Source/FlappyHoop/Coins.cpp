// Fill out your copyright notice in the Description page of Project Settings.


#include "Coins.h"
#include "BallInterface.h"
#include "GameModeInterface.h"
#include "FunctionsLibrary.h"
#include "Engine/TargetPoint.h"

// Sets default values
ACoins::ACoins()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	CoinMesh = CreateDefaultSubobject< UStaticMeshComponent>(TEXT("CoinMesh"));
	RootComponent = CoinMesh;
	CoinMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CoinMesh->SetCollisionResponseToAllChannels(ECR_Overlap);
	CoinMesh->SetRelativeRotation(FRotator(19.0f, 0.0f, 90.0f));
	CoinMesh->SetRelativeScale3D(FVector(0.75f, 0.75f, 0.25f));

	ActivateCoin(false);
}

// Called when the game starts or when spawned
void ACoins::BeginPlay()
{
	Super::BeginPlay();
	
	GameModeInterface = UFunctionsLibrary::GetGameModeInterface(this);

	CoinMesh->OnComponentBeginOverlap.AddDynamic(this, &ACoins::OnCoinBeginOverlap);
}

// Called every frame
void ACoins::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ACoins::OnCoinBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor->GetClass()->ImplementsInterface(UBallInterface::StaticClass()))
	{
		if (GameModeInterface)
		{
			ActivateCoin(false);
			GameModeInterface->AddCoin();
			GameModeInterface->OnCoinCollectedDelegate().Broadcast();
		}
	}
}

void ACoins::ActivateCoin(bool bActivate)
{
	bActive = bActivate;
	SetActorHiddenInGame(!bActivate);
	SetActorEnableCollision(bActivate);

	if (bActivate)
	{
		int32 RandomIndex = FMath::RandRange(0, CoinLocations.Num() - 1);
		FVector NewLocation = CoinLocations[RandomIndex]->GetActorLocation();
		SetActorLocation(NewLocation);
	}
}

