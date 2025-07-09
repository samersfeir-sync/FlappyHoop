// Fill out your copyright notice in the Description page of Project Settings.


#include "Gems.h"
#include "GameModeInterface.h"
#include "BallInterface.h"
#include "Kismet/GameplayStatics.h"
#include "GameInstanceInterface.h"

AGems::AGems()
{
	CoinMesh->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
	CoinMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
}

void AGems::BeginPlay()
{
	Super::BeginPlay();

	CoinMesh->OnComponentBeginOverlap.Clear();
	CoinMesh->OnComponentBeginOverlap.AddDynamic(this, &AGems::OnCoinBeginOverlap);
}

void AGems::OnCoinBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor->GetClass()->ImplementsInterface(UBallInterface::StaticClass()))
	{
		if (GameModeInterface)
		{
			ActivateCoin(false);
			GameModeInterface->AddGem();
			GameModeInterface->OnCoinCollectedDelegate().Broadcast(false);
		}

		UGameplayStatics::PlaySound2D(this, PickUpSound);
	}
}
