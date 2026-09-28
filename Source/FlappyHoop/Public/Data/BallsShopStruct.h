#pragma once

#include "CoreMinimal.h"
#include "Data/BallType.h"
#include "Engine/Texture2D.h"
#include "BallsShopStruct.generated.h"

USTRUCT(BlueprintType)
struct FBallsShopStruct
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, Category = "Shop")
    EBallType BallType = EBallType::Basketball;

    UPROPERTY(EditDefaultsOnly, Category = "Shop")
    bool IsPurchased = false;

    UPROPERTY(EditDefaultsOnly, Category = "Shop")
    TObjectPtr<UTexture2D> BallTexture = nullptr;

    UPROPERTY(EditDefaultsOnly, Category = "Shop")
    int32 Price = 200;

    bool operator==(const FBallsShopStruct& Other) const
    {
        return BallType == Other.BallType;
    }
};
