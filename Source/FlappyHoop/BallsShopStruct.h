// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BallType.h"
#include "BallsShopStruct.generated.h"

USTRUCT(BlueprintType)
struct FBallsShopStruct
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, Category = "Shop")
    EBallType BallType;

    UPROPERTY(EditDefaultsOnly, Category = "Shop")
    bool IsPurchased = false;

    UPROPERTY(EditDefaultsOnly, Category = "Shop")
    UTexture2D* BallTexture = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Shop")
    int Price = 200;
};