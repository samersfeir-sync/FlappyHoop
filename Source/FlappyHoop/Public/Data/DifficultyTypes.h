// Difficulty modes (Easy / Hard) and the per-mode tuning that ramps up level by level during a run.

#pragma once

#include "CoreMinimal.h"
#include "DifficultyTypes.generated.h"

UENUM(BlueprintType)
enum class EDifficultyMode : uint8
{
	Easy,
	Hard
};

/** A value at level 1 and at the profile's max level. Levels in between are interpolated linearly. */
USTRUCT(BlueprintType)
struct FDifficultyRange
{
	GENERATED_BODY()

	FDifficultyRange() = default;
	FDifficultyRange(float InAtStart, float InAtMaxLevel) : AtStart(InAtStart), AtMaxLevel(InAtMaxLevel) {}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Difficulty")
	float AtStart = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Difficulty")
	float AtMaxLevel = 0.0f;

	/** Alpha 0 = level 1, 1 = max level. */
	float Evaluate(float Alpha) const { return FMath::Lerp(AtStart, AtMaxLevel, FMath::Clamp(Alpha, 0.0f, 1.0f)); }
};

/**
 * Everything one difficulty mode changes. Edited on BP_BaseGameMode (Difficulty > Easy Profile / Hard Profile).
 * "Range" values move from AtStart to AtMaxLevel as the player scores (one level every BasketsPerLevel baskets).
 */
USTRUCT(BlueprintType)
struct FDifficultyProfile
{
	GENERATED_BODY()

	// --- Ramp ---

	/** A new level every this many baskets. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ramp", meta = (ClampMin = "1"))
	int32 BasketsPerLevel = 5;

	/** Levels stop rising here (level 1 is the start). Range values reach AtMaxLevel at this level. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ramp", meta = (ClampMin = "1"))
	int32 MaxLevel = 6;

	// --- Hoop ---

	/** True: the hoop picks one of the three arrows (Up/Middle/Down) like before. False: it stays in a band around Middle. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hoop")
	bool bUseAllHoopHeights = false;

	/** Only when bUseAllHoopHeights is off. 0 = always the Middle arrow, 1 = anywhere between the Down and Up arrows. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hoop")
	FDifficultyRange HoopHeightBand = FDifficultyRange(0.35f, 0.75f);

	/** How far the hoop drifts up and down (cm). 0 = the hoop stays still. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hoop")
	FDifficultyRange HoopDriftAmplitude = FDifficultyRange(0.0f, 0.0f);

	/** Full up-and-down cycles per second. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hoop")
	FDifficultyRange HoopDriftCyclesPerSecond = FDifficultyRange(0.15f, 0.35f);

	// --- Ball ---

	/**
	 * Multiplies BP_Ball's gravity. Lower = floatier. To keep the jump height the same, keep FlapStrength
	 * at about the square root of this (0.8 -> 0.9, 1.2 -> 1.1).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball")
	FDifficultyRange GravityScale = FDifficultyRange(1.0f, 1.0f);

	/** Multiplies BP_Ball's VerticalImpulse (the tap). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball")
	FDifficultyRange FlapStrength = FDifficultyRange(1.0f, 1.0f);

	/** The ball never falls faster than this (cm/s). 0 = no limit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball")
	FDifficultyRange MaxFallSpeed = FDifficultyRange(0.0f, 0.0f);

	// --- Round timer ---

	/** Round length at the start of a run (seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timer", meta = (ClampMin = "1.0"))
	float StartRoundTime = 10.0f;

	/** Each basket shortens the round by this fraction of StartRoundTime. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timer", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RoundShrinkPerBasket = 0.05f;

	/** The round never gets shorter than this (seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timer", meta = (ClampMin = "0.5"))
	float MinRoundTime = 3.0f;

	/** Time given back per basket, as a fraction of the round length (1 = full refill). Capped at a full bar. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timer")
	FDifficultyRange TimeBackFraction = FDifficultyRange(1.0f, 0.8f);

	/** Extra time back for a swish (fraction of the round length). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timer", meta = (ClampMin = "0.0"))
	float SwishTimeBonusFraction = 0.25f;

	// --- Combo ---

	/** True: only swishes grow the combo and refill the combo bar. Normal baskets still score with the current combo. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
	bool bOnlySwishBuildsCombo = false;

	/** Seconds before an unrefreshed combo expires. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
	FDifficultyRange ComboWindowSeconds = FDifficultyRange(6.0f, 4.0f);

	// --- Rewards ---

	/** Chance (%) that a coin or gem appears after a basket. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rewards")
	FDifficultyRange CollectibleChancePercent = FDifficultyRange(25.0f, 35.0f);

	/** When something appears, chance (%) that it is a gem instead of a coin. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rewards")
	FDifficultyRange GemSharePercent = FDifficultyRange(25.0f, 30.0f);

	static FDifficultyProfile MakeEasy()
	{
		FDifficultyProfile P;
		P.BasketsPerLevel = 6;
		P.MaxLevel = 5;
		P.bUseAllHoopHeights = false;
		P.HoopHeightBand = FDifficultyRange(0.35f, 0.75f);
		P.HoopDriftAmplitude = FDifficultyRange(0.0f, 0.0f);
		P.GravityScale = FDifficultyRange(0.8f, 0.9f);
		P.FlapStrength = FDifficultyRange(0.9f, 0.95f);
		P.MaxFallSpeed = FDifficultyRange(1000.0f, 1400.0f);
		P.StartRoundTime = 12.0f;
		P.RoundShrinkPerBasket = 0.04f;
		P.MinRoundTime = 5.0f;
		P.TimeBackFraction = FDifficultyRange(1.0f, 0.8f);
		P.SwishTimeBonusFraction = 0.25f;
		P.bOnlySwishBuildsCombo = false;
		P.ComboWindowSeconds = FDifficultyRange(6.0f, 4.0f);
		P.CollectibleChancePercent = FDifficultyRange(25.0f, 35.0f);
		P.GemSharePercent = FDifficultyRange(25.0f, 30.0f);
		return P;
	}

	static FDifficultyProfile MakeHard()
	{
		FDifficultyProfile P;
		P.BasketsPerLevel = 5;
		P.MaxLevel = 6;
		P.bUseAllHoopHeights = true;
		P.HoopDriftAmplitude = FDifficultyRange(40.0f, 140.0f);
		P.HoopDriftCyclesPerSecond = FDifficultyRange(0.15f, 0.35f);
		P.GravityScale = FDifficultyRange(1.15f, 1.3f);
		P.FlapStrength = FDifficultyRange(1.07f, 1.14f);
		P.MaxFallSpeed = FDifficultyRange(0.0f, 0.0f);
		P.StartRoundTime = 10.0f;
		P.RoundShrinkPerBasket = 0.05f;
		P.MinRoundTime = 3.0f;
		P.TimeBackFraction = FDifficultyRange(0.7f, 0.5f);
		P.SwishTimeBonusFraction = 0.2f;
		P.bOnlySwishBuildsCombo = true;
		P.ComboWindowSeconds = FDifficultyRange(4.0f, 2.5f);
		P.CollectibleChancePercent = FDifficultyRange(30.0f, 45.0f);
		P.GemSharePercent = FDifficultyRange(25.0f, 35.0f);
		return P;
	}
};
