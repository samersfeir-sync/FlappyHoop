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

    bool operator==(const FBallsShopStruct& Other) const
    {
        return BallType == Other.BallType;
    }
};
