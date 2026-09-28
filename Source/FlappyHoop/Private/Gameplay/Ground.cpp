// Fill out your copyright notice in the Description page of Project Settings.


#include "Gameplay/Ground.h"
#include "Gameplay/BallInterface.h"
#include "Framework/BaseGameMode.h"
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

	GameMode = ABaseGameMode::Get(this);

	if (GameMode)
	{
		OnActorHit.AddUniqueDynamic(this, &AGround::OnHit);
	}
}

void AGround::OnHit(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit)
{
	if (GameMode && OtherActor && OtherActor->Implements<UBallInterface>())
	{
		// Ignored unless the clock has run out; the phase change makes repeat bounces harmless.
		GameMode->NotifyBallLanded();
	}
}
