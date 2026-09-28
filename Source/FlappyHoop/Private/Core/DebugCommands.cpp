// Fill out your copyright notice in the Description page of Project Settings.

// Debug console commands (open the console with ~ in the editor/PC, four-finger tap on Android dev builds).
// Compiled out of Shipping builds, so they never reach players.

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "Core/FlappyHoop.h"
#include "Data/UserProgression.h"
#include "Systems/BallCatalogSubsystem.h"
#include "Systems/EconomySubsystem.h"
#include "Systems/SaveSubsystem.h"
#include "HAL/IConsoleManager.h"
#include "Engine/World.h"

namespace FlappyHoopDebug
{
	/** Reads the first argument as a whole number. Returns false (and logs) if it's missing or not a number. */
	static bool ParseAmount(const TArray<FString>& Args, const TCHAR* Command, int32& OutAmount)
	{
		if (Args.Num() < 1 || !Args[0].IsNumeric())
		{
			UE_LOG(LogFlappyHoop, Warning, TEXT("Usage: %s <amount>   (negative removes)"), Command);
			return false;
		}
		OutAmount = FCString::Atoi(*Args[0]);
		return true;
	}

	/** Positive adds, negative removes (never below 0). */
	static void ChangeBalance(UWorld* World, const TArray<FString>& Args, bool bGems)
	{
		const TCHAR* Command = bGems ? TEXT("AddGems") : TEXT("AddCoins");
		int32 Amount = 0;
		UEconomySubsystem* Economy = UEconomySubsystem::Get(World);
		if (!Economy || !ParseAmount(Args, Command, Amount))
		{
			return;
		}

		if (Amount >= 0)
		{
			bGems ? Economy->AddGems(Amount) : Economy->AddCoins(Amount);
		}
		else
		{
			const int32 Remove = FMath::Min(-Amount, bGems ? Economy->GetGems() : Economy->GetCoins());
			bGems ? Economy->TrySpend(0, Remove) : Economy->TrySpend(Remove, 0);
		}

		UE_LOG(LogFlappyHoop, Display, TEXT("%s %d -> coins %d, gems %d"), Command, Amount, Economy->GetCoins(), Economy->GetGems());
	}

	static FAutoConsoleCommandWithWorldAndArgs AddCoinsCommand(
		TEXT("AddCoins"),
		TEXT("Debug: AddCoins <amount>. Adds (or with a negative number removes) banked coins."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
		{
			ChangeBalance(World, Args, false);
		}));

	static FAutoConsoleCommandWithWorldAndArgs AddGemsCommand(
		TEXT("AddGems"),
		TEXT("Debug: AddGems <amount>. Adds (or with a negative number removes) banked gems."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
		{
			ChangeBalance(World, Args, true);
		}));

	static FAutoConsoleCommandWithWorldAndArgs UnlockAllBallsCommand(
		TEXT("UnlockAllBalls"),
		TEXT("Debug: unlocks every ball in the catalog for free (keeps the current selection)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
		{
			UBallCatalogSubsystem* Catalog = UBallCatalogSubsystem::Get(World);
			if (!Catalog)
			{
				return;
			}

			const EBallType Selected = Catalog->GetSelected();
			for (const FBallCatalogEntry& Entry : Catalog->GetBalls())
			{
				Catalog->Unlock(Entry.BallType); // no-op for balls already owned
			}
			Catalog->Select(Selected);
			UE_LOG(LogFlappyHoop, Display, TEXT("UnlockAllBalls: done"));
		}));

	static FAutoConsoleCommandWithWorldAndArgs ResetProgressCommand(
		TEXT("ResetProgress"),
		TEXT("Debug: wipes the save (coins, gems, balls, high score, settings, no-ads). Restart the game afterwards."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
		{
			USaveSubsystem* Save = USaveSubsystem::Get(World);
			if (!Save)
			{
				return;
			}

			FUserProgression Fresh;
			Fresh.SaveVersion = Save->GetProgression().SaveVersion; // already in the new format, skip migration
			Save->Save(Fresh);
			UE_LOG(LogFlappyHoop, Display, TEXT("ResetProgress: save wiped. Restart the game to see a clean state."));
		}));
}

#endif // !UE_BUILD_SHIPPING
