#pragma once

#include "CoreMinimal.h"
#include "BallType.generated.h"

UENUM(BlueprintType)
enum class EBallType : uint8
{
	Basketball     UMETA(DisplayName = "Basketball"),
	SoccerBall     UMETA(DisplayName = "Soccer Ball"),
	BeachBall      UMETA(DisplayName = "Beach Ball"),
	VolleyBall	   UMETA(DisplayName = "Volley Ball"),
	TennisBall     UMETA(DisplayName = "Tennis Ball"),
	Baseball       UMETA(DisplayName = "Baseball"),
	BowlingBall    UMETA(DisplayName = "Bowling Ball"),
	RugbyBall	   UMETA(DisplayName = "RugbyBall"),
	Tomato   	   UMETA(DisplayName = "Tomato")
};