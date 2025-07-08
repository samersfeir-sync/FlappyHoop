// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Coins.h"
#include "Gems.generated.h"

/**
 * 
 */
UCLASS()
class FLAPPYHOOP_API AGems : public ACoins
{
	GENERATED_BODY()
	
public:

	AGems();

private:

	virtual void BeginPlay() override;

	virtual void OnCoinBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;
};
