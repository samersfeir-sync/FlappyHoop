// Fill out your copyright notice in the Description page of Project Settings.

// Debug console commands (open the console with ~ in the editor/PC, four-finger tap on Android dev builds).
// Compiled out of Shipping builds, so they never reach players.

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "Core/FlappyHoop.h"
#include "Data/UserProgression.h"
#include "Framework/BaseGameMode.h"
#include "Systems/BallCatalogSubsystem.h"
#include "Systems/EconomySubsystem.h"
#include "Systems/SaveSubsystem.h"
#include "Systems/CloudSaveSubsystem.h"
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

	static FAutoConsoleCommandWithWorldAndArgs CloudSyncCommand(
		TEXT("CloudSync"),
		TEXT("Debug: merges local progress with the cloud save now (Restore Purchases does the same). In the editor, turn on bSimulateCloudSave on BP_GameInfoInstance."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
		{
			if (UCloudSaveSubsystem* CloudSave = UCloudSaveSubsystem::Get(World))
			{
				CloudSave->RestoreNow();
				UE_LOG(LogFlappyHoop, Display, TEXT("CloudSync: started (see [CloudSave] lines)"));
			}
		}));

	static FAutoConsoleCommandWithWorldAndArgs CloudStatusCommand(
		TEXT("CloudStatus"),
		TEXT("Debug: prints the local balances and the cloud-sync bookkeeping."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
		{
			const USaveSubsystem* Save = USaveSubsystem::Get(World);
			const UCloudSaveSubsystem* CloudSave = UCloudSaveSubsystem::Get(World);
			if (!Save)
			{
				return;
			}
			const FUserProgression& P = Save->GetProgression();
			UE_LOG(LogFlappyHoop, Display, TEXT("CloudStatus: supported=%d synced-once=%d coins=%d (base %d) gems=%d (base %d) balls=%d selectedAt=%lld lastResult=%d"),
				(CloudSave && CloudSave->IsSupported()) ? 1 : 0, P.bCloudSynced ? 1 : 0, P.TotalCoins, P.CloudBaseCoins, P.TotalGems, P.CloudBaseGems,
				P.OwnedBalls.Num(), P.BallSelectedAt, CloudSave ? static_cast<int32>(CloudSave->GetLastResult()) : -1);
		}));

	static FAutoConsoleCommandWithWorldAndArgs SetDifficultyCommand(
		TEXT("SetDifficulty"),
		TEXT("Debug: SetDifficulty Easy|Hard. Only works in the main menu (same as the menu buttons)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
		{
			ABaseGameMode* GameMode = ABaseGameMode::Get(World);
			if (!GameMode || Args.Num() < 1)
			{
				UE_LOG(LogFlappyHoop, Warning, TEXT("Usage: SetDifficulty Easy|Hard"));
				return;
			}

			const EDifficultyMode Mode = Args[0].Equals(TEXT("Hard"), ESearchCase::IgnoreCase) ? EDifficultyMode::Hard : EDifficultyMode::Easy;
			const bool bChanged = GameMode->SetDifficulty(Mode);
			UE_LOG(LogFlappyHoop, Display, TEXT("SetDifficulty: %s"), bChanged
				? (Mode == EDifficultyMode::Hard ? TEXT("Hard") : TEXT("Easy"))
				: TEXT("not in the main menu, ignored"));
		}));
}

#endif // !UE_BUILD_SHIPPING
