// Fill out your copyright notice in the Description page of Project Settings.


#include "Gameplay/Gems.h"
#include "Framework/BaseGameMode.h"
#include "Components/StaticMeshComponent.h"

AGems::AGems()
{
	CoinMesh->SetRelativeRotation(FRotator::ZeroRotator);
	CoinMesh->SetRelativeScale3D(FVector::OneVector);
}

void AGems::HandleCollected()
{
	if (GameMode)
	{
		GameMode->AddGem();
		GameMode->OnCoinCollected.Broadcast(false);
	}
}
