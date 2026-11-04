#pragma once

#include "CoreMinimal.h"
#include "Data/BallsShopStruct.h"
#include "Data/BallType.h"
#include "Data/DifficultyTypes.h"
#include "UserProgression.generated.h"

USTRUCT(BlueprintType)
struct FUserProgression
{
    GENERATED_BODY()

    /** Best score on Easy (saves from before difficulty modes land here too). */
    UPROPERTY(EditDefaultsOnly, Category = "Progression")
    int32 HighScore = 0;

    /** Best score on Hard. */
    UPROPERTY(EditDefaultsOnly, Category = "Progression")
    int32 HighScoreHard = 0;

    /** Last difficulty picked in the main menu. */
    UPROPERTY(EditDefaultsOnly, Category = "Preference")
    EDifficultyMode Difficulty = EDifficultyMode::Easy;

    UPROPERTY(EditDefaultsOnly, Category = "Collectibles")
    int32 TotalCoins = 0;

    UPROPERTY(EditDefaultsOnly, Category = "Collectibles")
    int32 TotalGems = 0;

    /** Balls the player bought. Balls marked "owned by default" in the catalog don't need to be listed. */
    UPROPERTY(EditDefaultsOnly, Category = "Progression")
    TArray<EBallType> OwnedBalls;

    /** Selected ball. */
    UPROPERTY(EditDefaultsOnly, Category = "Preference")
    EBallType BallType = EBallType::Basketball;

    /** When BallType was last chosen (Unix seconds, 0 = never). Cloud sync keeps the most recent selection. */
    UPROPERTY()
    int64 BallSelectedAt = 0;

    /**
     * OLD format (save version 0): the full shop list with textures/prices. Converted into OwnedBalls on load
     * and then emptied. On the save-game Blueprint's defaults it is still the fallback shop list until a
     * BallCatalog asset is assigned on BP_GameInfoInstance.
     */
    UPROPERTY(EditDefaultsOnly, Category = "Legacy")
    TArray<FBallsShopStruct> BallsOwned;

    /** 0 = old format, 1 = OwnedBalls. */
    UPROPERTY()
    int32 SaveVersion = 0;

    UPROPERTY(EditDefaultsOnly, Category = "Sound Settings")
    bool bIsMusicMuted = false;

    UPROPERTY(EditDefaultsOnly, Category = "Sound Settings")
    bool bIsSFXMuted = false;

    UPROPERTY(EditDefaultsOnly, Category = "Ads")
    bool bNoAds = false;

    // --- Cloud save bookkeeping (UCloudSaveSubsystem) ---

    /** True once this device has merged with the cloud copy at least once. */
    UPROPERTY()
    bool bCloudSynced = false;

    /** Coins / gems the cloud was last known to hold. Local changes since then are added on the next sync. */
    UPROPERTY()
    int32 CloudBaseCoins = 0;

    UPROPERTY()
    int32 CloudBaseGems = 0;
};
