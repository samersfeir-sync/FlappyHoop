// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Gameplay/Coins.h"
#include "Gems.generated.h"

UCLASS()
class FLAPPYHOOP_API AGems : public ACoins
{
	GENERATED_BODY()

public:

	AGems();

protected:

	virtual void HandleCollected() override;
};
