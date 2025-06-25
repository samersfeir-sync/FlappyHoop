#pragma once

#include "CoreMinimal.h"
#include "BallsShopStruct.h"
#include "BallType.h"
#include "UserProgression.generated.h"

USTRUCT(BlueprintType)
struct FUserProgression
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly)
    int32 HighScore = 0;

    UPROPERTY(EditDefaultsOnly)
    int32 TotalCoins = 0;

    UPROPERTY(EditDefaultsOnly)
    TArray<FBallsShopStruct> BallsOwned;

    UPROPERTY(EditDefaultsOnly)
    EBallType BallType;
};
