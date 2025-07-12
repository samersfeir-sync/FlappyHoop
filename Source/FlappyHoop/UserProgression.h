#pragma once

#include "CoreMinimal.h"
#include "BallsShopStruct.h"
#include "BallType.h"
#include "UserProgression.generated.h"

USTRUCT(BlueprintType)
struct FUserProgression
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, Category = "Progression")
    int32 HighScore = 0;

    UPROPERTY(EditDefaultsOnly, Category = "Collectibles")
    int32 TotalCoins = 0;

    UPROPERTY(EditDefaultsOnly, Category = "Collectibles")
    int32 TotalGems = 0;

    UPROPERTY(EditDefaultsOnly, Category = "Progression")
    TArray<FBallsShopStruct> BallsOwned;

    UPROPERTY(EditDefaultsOnly, Category = "Preference")
    EBallType BallType;

    UPROPERTY(EditDefaultsOnly, Category = "Sound Settings")
    bool bIsMusicMuted = false;

    UPROPERTY(EditDefaultsOnly, Category = "Sound Settings")
	bool bIsSFXMuted = false;

    UPROPERTY(EditDefaultsOnly, Category = "Ads")
	bool bNoAds = false;
};
