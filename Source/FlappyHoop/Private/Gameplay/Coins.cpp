// Fill out your copyright notice in the Description page of Project Settings.


#include "Gameplay/Coins.h"
#include "Core/FlappyHoop.h"
#include "Gameplay/BallInterface.h"
#include "Framework/BaseGameMode.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/TargetPoint.h"
#include "Kismet/GameplayStatics.h"

ACoins::ACoins()
{
	PrimaryActorTick.bCanEverTick = false;

	CoinMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoinMesh"));
	RootComponent = CoinMesh;
	CoinMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CoinMesh->SetCollisionResponseToAllChannels(ECR_Overlap);
	CoinMesh->SetGenerateOverlapEvents(true);
	CoinMesh->SetRelativeRotation(FRotator(19.0f, 0.0f, 90.0f));
	CoinMesh->SetRelativeScale3D(FVector(0.75f, 0.75f, 0.25f));

	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}

void ACoins::BeginPlay()
{
	Super::BeginPlay();

	GameMode = ABaseGameMode::Get(this);

	if (GameMode)
	{
		GameMode->OnGameReset.AddUObject(this, &ACoins::ResetCoin);
	}

	CoinMesh->OnComponentBeginOverlap.AddUniqueDynamic(this, &ACoins::OnCoinBeginOverlap);

	CoinLocations.RemoveAll([](const TObjectPtr<ATargetPoint>& Point) { return Point == nullptr; });
	if (CoinLocations.IsEmpty())
	{
		UE_LOG(LogFlappyHoop, Warning, TEXT("%s has no CoinLocations set - it will never spawn."), *GetName());
	}
}

void ACoins::OnCoinBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bActive || !OtherActor || !OtherActor->Implements<UBallInterface>())
	{
		return;
	}

	ActivateCoin(false);
	HandleCollected();
	UGameplayStatics::PlaySound2D(this, PickUpSound);
}

void ACoins::HandleCollected()
{
	if (GameMode)
	{
		GameMode->AddCoin();
		GameMode->OnCoinCollected.Broadcast(true);
	}
}

void ACoins::ActivateCoin(bool bActivate)
{
	// Guard: RandRange(0, -1) on an empty array used to index out of bounds and crash.
	if (bActivate && CoinLocations.IsEmpty())
	{
		return;
	}

	bActive = bActivate;
	SetActorHiddenInGame(!bActivate);
	SetActorEnableCollision(bActivate);

	if (bActivate)
	{
		const int32 RandomIndex = FMath::RandRange(0, CoinLocations.Num() - 1);
		SetActorLocation(CoinLocations[RandomIndex]->GetActorLocation());
	}
}
