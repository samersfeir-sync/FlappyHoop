// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Ticker.h"
#include "Data/UserProgression.h"
#include "CloudSaveSubsystem.generated.h"

/** Result of a cloud sync / restore. */
enum class EFlappyCloudSyncResult : uint8
{
	/** Local progress and the cloud copy are merged and up to date. */
	Synced,
	/** Android: not signed in to Google Play Games (or the player cancelled sign-in). */
	NotSignedIn,
	/** iOS: no iCloud account on the device. Progress is kept locally and synced once the player signs in. */
	NoICloudAccount,
	/** The service answered with an error or didn't answer in time. */
	Failed,
	/** No cloud service on this platform (editor/desktop without simulation) or cloud save is turned off. */
	Unsupported
};

/** Fired when a sync finishes (whatever started it). */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCloudSyncFinished, EFlappyCloudSyncResult /*Result*/);

/** Fired after cloud data changed the local progression (gems, coins, balls, selection, high scores, no-ads). */
DECLARE_MULTICAST_DELEGATE(FOnCloudProgressApplied);

/**
 * The part of the progression kept in the cloud (plain C++, JSON on the wire).
 * Balls are stored by enum name; names this build doesn't know (balls added in a newer version) are kept untouched.
 */
struct FFlappyCloudData
{
	int32 Coins = 0;
	int32 Gems = 0;
	int32 HighScore = 0;
	int32 HighScoreHard = 0;
	bool bNoAds = false;
	TSet<FString> OwnedBalls;
	FString SelectedBall;
	/** When the selection was made (Unix seconds). The newer selection wins. */
	int64 SelectedAt = 0;

	FString ToJson() const;
	static bool FromJson(const FString& Json, FFlappyCloudData& Out);
};

/**
 * Cloud save + restore of the player's progress: gems (bought or earned), coins, owned and selected balls,
 * high scores and "remove ads".
 *
 * Backends:
 * - Android: Google Play Games Services Saved Games (snapshot "flappyhoop_progress"), needs PGS sign-in
 *   (ULeaderboardSubsystem). Java side in FlappyHoop_UPL.xml.
 * - iOS: iCloud key-value storage (NSUbiquitousKeyValueStore). No Game Center needed, just an iCloud account.
 * - Editor/desktop (non-Shipping, when bSimulateCloudSave is on): a JSON file in Saved/ stands in for the cloud.
 *
 * Merge (three-way, so nothing is lost or counted twice when several devices or a reinstall are involved):
 * - coins/gems: cloud value + what changed locally since the last sync (earned, bought or spent);
 * - owned balls: union (never lost); selected ball: the most recent selection; high scores: max; no-ads: either.
 *
 * Syncs: at startup / after sign-in, a few seconds after any change, right after a gem purchase is credited,
 * when the app goes to the background, and when the player presses Restore Purchases.
 */
UCLASS()
class FLAPPYHOOP_API UCloudSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	static UCloudSaveSubsystem* Get(const UObject* WorldContextObject);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** True when a cloud backend exists on this platform and cloud save is turned on. */
	bool IsSupported() const;

	/** Merges local progress with the cloud now (no-op if a sync is already running; it then runs once more after). */
	void SyncNow();

	/**
	 * Player-initiated restore (Settings > Restore Purchases): signs in to Google Play Games first if needed,
	 * then syncs. The answer arrives through OnSyncFinished.
	 */
	void RestoreNow();

	bool IsSyncInProgress() const { return bSyncInFlight; }
	EFlappyCloudSyncResult GetLastResult() const { return LastResult; }

	FOnCloudSyncFinished OnSyncFinished;
	FOnCloudProgressApplied OnCloudProgressApplied;

	/**
	 * Pure merge (no side effects, used by every backend).
	 * @param Local      current local progression
	 * @param Cloud      what the cloud holds (nullptr = nothing there yet)
	 * @param Defaults   a brand-new save's progression (baseline for a device that never synced)
	 * @param OutLocal   merged local progression (cloud baseline set to the cloud values that were read)
	 * @param OutCloud   what to write back to the cloud
	 */
	static void Merge(const FUserProgression& Local, const FFlappyCloudData* Cloud, const FUserProgression& Defaults,
		FUserProgression& OutLocal, FFlappyCloudData& OutCloud);

	// --- Backend callbacks (game thread). Public so the platform glue can reach them. ---
	void HandleCloudLoaded(int64 RequestId, bool bSuccess, const FString& Json);
	void HandleCloudCommitted(int64 RequestId, bool bSuccess);
	void HandleExternalCloudChange();

private:

	// --- Triggers ---
	void HandleProgressionSaved(const FUserProgression& Progression);
	void HandleSignInChanged(bool bSignedIn);
	void HandleSignInAttemptFinished(bool bSignedIn);
	void HandleEnterBackground();
	void ScheduleSync(float DelaySeconds);
	bool OnSyncTimer(float DeltaTime);
	bool OnWatchdog(float DeltaTime);

	// --- Sync steps ---
	void StartSync();
	void FinishSync(EFlappyCloudSyncResult Result);

	/** Merges what was read, applies it locally and returns the JSON to write back. */
	FString MergeAndApply(const FFlappyCloudData* Cloud);

	/** After a successful write: the written values become the new baseline. */
	void MarkCommitted();

	FUserProgression GetDefaultProgression() const;

	/** Fingerprint of the cloud-relevant fields, to tell whether a local save needs a sync. */
	static FString CloudFingerprint(const FUserProgression& Progression);

	// --- Platform backends: begin a read; the answer comes back through HandleCloudLoaded. ---
	void BackendLoad(int64 RequestId);
	void BackendCommit(int64 RequestId, const FString& Json);
	void BackendDiscard();

	bool bSyncInFlight = false;
	bool bSyncQueued = false;
	bool bRestoreRequested = false;
	int64 CurrentRequestId = 0;
	FFlappyCloudData PendingCommit;
	bool bHasPendingCommit = false;
	/** iOS: no iCloud account signed in during this sync (reported to the UI; data stays on the device). */
	bool bNoCloudAccount = false;

	FString LastSyncedFingerprint;
	EFlappyCloudSyncResult LastResult = EFlappyCloudSyncResult::Unsupported;

	FTSTicker::FDelegateHandle SyncTimerHandle;
	FTSTicker::FDelegateHandle WatchdogHandle;

	/** Applying cloud data saves the progression too; don't treat that save as a new local change. */
	bool bApplyingCloudData = false;

#if PLATFORM_IOS
	void* ICloudObserver = nullptr; // id<NSObject>, retained
#endif
};
