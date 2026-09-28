#pragma once

#include "CoreMinimal.h"
#include "GameStateEnum.generated.h"

/**
 * Phase of a run. Owned by ABaseGameMode; everything else reacts to OnRunPhaseChanged.
 *
 *   MainMenu --Play--> Playing --time up + ball lands--> SecondChance --ad/gems--> Playing
 *                         |                                  |
 *                         +-----------(can't continue)-------+--skip/timeout--> GameOver --Home--> MainMenu
 */
UENUM(BlueprintType)
enum class EGameStateEnum : uint8
{
	MainMenu     UMETA(DisplayName = "MainMenu"),
	Playing      UMETA(DisplayName = "Playing"),
	SecondChance UMETA(DisplayName = "Second Chance"),
	GameOver     UMETA(DisplayName = "Game Over")
};
