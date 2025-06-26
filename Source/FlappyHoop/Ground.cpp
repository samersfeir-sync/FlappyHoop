// Fill out your copyright notice in the Description page of Project Settings.


#include "Ground.h"
#include "Ball.h"
#include "FunctionsLibrary.h"
#include "GameModeInterface.h"
#include "Components/BoxComponent.h"

AGround::AGround()
{
	PrimaryActorTick.bCanEverTick = false;

	Ground = CreateDefaultSubobject<UBoxComponent>(TEXT("Ground"));
	RootComponent = Ground;

	Ground->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Ground->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	Ground->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
}

void AGround::BeginPlay()
{
	Super::BeginPlay();

	GameModeInterface = UFunctionsLibrary::GetGameModeInterface(this);

	if (GameModeInterface)
	{
		GameModeInterface->OnGameResetDelegate().AddUObject(this, &AGround::ResetGround);
		OnActorHit.AddDynamic(this, &AGround::OnHit);
	}
}

void AGround::OnHit(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit)
{
	if (bAlreadyProcessedHit) return;

	if (OtherActor->IsA(ABall::StaticClass()))
	{
		bool bTimeEnded = GameModeInterface->GetTimeEndedBool();

		if (bTimeEnded)
		{
			bAlreadyProcessedHit = true;
			GameModeInterface->EndGame();
		}
	}
}

void AGround::ResetGround()
{
	bAlreadyProcessedHit = false;
}
