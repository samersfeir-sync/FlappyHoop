#pragma once

#include "CoreMinimal.h"
#include "BallsShopStruct.h"
#include "UserProgression.generated.h"

USTRUCT(BlueprintType)
struct FUserProgression
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly)
    int32 HighScore = 0;

    UPROPERTY(EditDefaultsOnly)
    TArray<FBallsShopStruct> BallsOwned;
};
