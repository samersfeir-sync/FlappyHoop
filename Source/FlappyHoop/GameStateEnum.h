#pragma once

#include "CoreMinimal.h"
#include "GameStateEnum.generated.h"

UENUM(BlueprintType)
enum class EGameStateEnum : uint8
{
	MainMenu     UMETA(DisplayName = "MainMenu"),
	Playing      UMETA(DisplayName = "Playing")
};