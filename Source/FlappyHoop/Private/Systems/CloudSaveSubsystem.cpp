// Fill out your copyright notice in the Description page of Project Settings.

#include "Systems/CloudSaveSubsystem.h"
#include "Core/FlappyHoop.h"
#include "Data/MySaveGame.h"
#include "Data/BallType.h"
#include "Framework/GameInfoInstance.h"
#include "Systems/SaveSubsystem.h"
#include "Systems/EconomySubsystem.h"
#include "Systems/BallCatalogSubsystem.h"
#include "Systems/LeaderboardSubsystem.h"
#include "Systems/StoreSubsystem.h"
#include "Async/Async.h"
#include "Misc/CoreDelegates.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

#if PLATFORM_ANDROID
#include "Android/AndroidApplication.h"
#include "Android/AndroidJNI.h"
#include "Android/AndroidJavaEnv.h"
#ifndef JNI_METHOD
#define JNI_METHOD __attribute__ ((visibility ("default"))) extern "C"
#endif
#endif

#if PLATFORM_IOS
#import <Foundation/Foundation.h>
#endif

namespace
{
	/** The subsystem the platform callbacks report to (read on the game thread only). */
	TWeakObjectPtr<UCloudSaveSubsystem> GActiveCloudSave;

	/** Cloud name of the save (Android snapshot name / iCloud key). Same on every platform. */
	const TCHAR* CloudSaveName = TEXT("flappyhoop_progress");

	/** Debounce after a local change (seconds). Gem increases (purchases) go up sooner. */
	constexpr float SyncDelayAfterChange = 4.0f;
	constexpr float SyncDelayAfterGemGain = 0.5f;

	/** Give up on a backend call that never answers. */
	constexpr float SyncTimeoutSeconds = 45.0f;

	constexpr int32 CloudFormatVersion = 1;

	FString BallName(EBallType Type)
	{
		return StaticEnum<EBallType>()->GetNameStringByValue(static_cast<int64>(Type));
	}

	bool BallFromName(const FString& Name, EBallType& Out)
	{
		const int64 Value = StaticEnum<EBallType>()->GetValueByNameString(Name);
		if (Value == INDEX_NONE)
		{
			return false;
		}
		Out = static_cast<EBallType>(Value);
		return true;
	}

	int32 ClampToInt32(int64 Value)
	{
		return static_cast<int32>(FMath::Clamp<int64>(Value, 0, MAX_int32));
	}

#if !UE_BUILD_SHIPPING && !PLATFORM_ANDROID && !PLATFORM_IOS
	FString SimulatedCloudFile()
	{
		return FPaths::ProjectSavedDir() / TEXT("SimulatedCloud") / (FString(CloudSaveName) + TEXT(".json"));
	}
#endif
}

// =====================================================================================================================
// Cloud data (JSON)
// =====================================================================================================================

FString FFlappyCloudData::ToJson() const
{
	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetNumberField(TEXT("v"), CloudFormatVersion);
	Root->SetNumberField(TEXT("coins"), Coins);
	Root->SetNumberField(TEXT("gems"), Gems);
	Root->SetNumberField(TEXT("hs"), HighScore);
	Root->SetNumberField(TEXT("hsHard"), HighScoreHard);
	Root->SetBoolField(TEXT("noAds"), bNoAds);
	Root->SetStringField(TEXT("ball"), SelectedBall);
	Root->SetNumberField(TEXT("ballAt"), static_cast<double>(SelectedAt));

	TArray<FString> Sorted = OwnedBalls.Array();
	Sorted.Sort();
	TArray<TSharedPtr<FJsonValue>> Balls;
	for (const FString& Name : Sorted)
	{
		Balls.Add(MakeShared<FJsonValueString>(Name));
	}
	Root->SetArrayField(TEXT("balls"), Balls);

	FString Out;
	const TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
		TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Out);
	FJsonSerializer::Serialize(Root, Writer);
	return Out;
}

bool FFlappyCloudData::FromJson(const FString& Json, FFlappyCloudData& Out)
{
	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<TCHAR>> Reader = TJsonReaderFactory<TCHAR>::Create(Json);
	if (Json.IsEmpty() || !FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		return false;
	}

	Out = FFlappyCloudData();

	double Number = 0.0;
	if (Root->TryGetNumberField(TEXT("coins"), Number))  { Out.Coins = ClampToInt32(static_cast<int64>(Number)); }
	if (Root->TryGetNumberField(TEXT("gems"), Number))   { Out.Gems = ClampToInt32(static_cast<int64>(Number)); }
	if (Root->TryGetNumberField(TEXT("hs"), Number))     { Out.HighScore = ClampToInt32(static_cast<int64>(Number)); }
	if (Root->TryGetNumberField(TEXT("hsHard"), Number)) { Out.HighScoreHard = ClampToInt32(static_cast<int64>(Number)); }
	if (Root->TryGetNumberField(TEXT("ballAt"), Number)) { Out.SelectedAt = static_cast<int64>(Number); }
	Root->TryGetBoolField(TEXT("noAds"), Out.bNoAds);
	Root->TryGetStringField(TEXT("ball"), Out.SelectedBall);

	const TArray<TSharedPtr<FJsonValue>>* Balls = nullptr;
	if (Root->TryGetArrayField(TEXT("balls"), Balls) && Balls)
	{
		for (const TSharedPtr<FJsonValue>& Value : *Balls)
		{
			FString Name;
			if (Value.IsValid() && Value->TryGetString(Name) && !Name.IsEmpty())
			{
				Out.OwnedBalls.Add(Name);
			}
		}
	}
	return true;
}

// =====================================================================================================================
// Merge
// =====================================================================================================================

void UCloudSaveSubsystem::Merge(const FUserProgression& Local, const FFlappyCloudData* Cloud, const FUserProgression& Defaults,
	FUserProgression& OutLocal, FFlappyCloudData& OutCloud)
{
	OutLocal = Local;

	if (Cloud)
	{
		// Coins/gems: the cloud's value plus what this device earned, bought or spent since it last saw the cloud.
		// A device that never synced compares against a brand-new save, so its whole balance counts as "earned here".
		const int64 BaseCoins = Local.bCloudSynced ? Local.CloudBaseCoins : Defaults.TotalCoins;
		const int64 BaseGems = Local.bCloudSynced ? Local.CloudBaseGems : Defaults.TotalGems;
		OutLocal.TotalCoins = ClampToInt32(int64(Cloud->Coins) + (int64(Local.TotalCoins) - BaseCoins));
		OutLocal.TotalGems = ClampToInt32(int64(Cloud->Gems) + (int64(Local.TotalGems) - BaseGems));

		// What the cloud is known to hold now. If writing back fails, the next sync still adds only the local delta.
		OutLocal.CloudBaseCoins = Cloud->Coins;
		OutLocal.CloudBaseGems = Cloud->Gems;
		OutLocal.bCloudSynced = true;

		OutLocal.HighScore = FMath::Max(Local.HighScore, Cloud->HighScore);
		OutLocal.HighScoreHard = FMath::Max(Local.HighScoreHard, Cloud->HighScoreHard);
		OutLocal.bNoAds = Local.bNoAds || Cloud->bNoAds;

		// Owned balls are never lost.
		for (const FString& Name : Cloud->OwnedBalls)
		{
			EBallType Type;
			if (BallFromName(Name, Type))
			{
				OutLocal.OwnedBalls.AddUnique(Type);
			}
		}

		// Selection: the most recent one. A device that never synced (fresh install) takes the cloud's on a tie.
		EBallType CloudBall;
		if (!Cloud->SelectedBall.IsEmpty() && BallFromName(Cloud->SelectedBall, CloudBall))
		{
			const bool bCloudNewer = Local.bCloudSynced
				? Cloud->SelectedAt > Local.BallSelectedAt
				: Cloud->SelectedAt >= Local.BallSelectedAt;
			if (bCloudNewer)
			{
				OutLocal.BallType = CloudBall;
				OutLocal.BallSelectedAt = Cloud->SelectedAt;
			}
		}
	}

	OutCloud = FFlappyCloudData();
	OutCloud.Coins = OutLocal.TotalCoins;
	OutCloud.Gems = OutLocal.TotalGems;
	OutCloud.HighScore = OutLocal.HighScore;
	OutCloud.HighScoreHard = OutLocal.HighScoreHard;
	OutCloud.bNoAds = OutLocal.bNoAds;
	OutCloud.SelectedBall = BallName(OutLocal.BallType);
	OutCloud.SelectedAt = OutLocal.BallSelectedAt;
	if (Cloud)
	{
		OutCloud.OwnedBalls = Cloud->OwnedBalls; // keeps balls this build doesn't know
	}
	for (const EBallType Type : OutLocal.OwnedBalls)
	{
		OutCloud.OwnedBalls.Add(BallName(Type));
	}
}

FString UCloudSaveSubsystem::CloudFingerprint(const FUserProgression& P)
{
	TArray<FString> Balls;
	for (const EBallType Type : P.OwnedBalls)
	{
		Balls.Add(BallName(Type));
	}
	Balls.Sort();

	return FString::Printf(TEXT("%d|%d|%d|%d|%d|%s|%lld|%s"),
		P.TotalCoins, P.TotalGems, P.HighScore, P.HighScoreHard, P.bNoAds ? 1 : 0,
		*BallName(P.BallType), P.BallSelectedAt, *FString::Join(Balls, TEXT(",")));
}

// =====================================================================================================================
// Subsystem
// =====================================================================================================================

UCloudSaveSubsystem* UCloudSaveSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UCloudSaveSubsystem>() : nullptr;
}

bool UCloudSaveSubsystem::IsSupported() const
{
	const UGameInfoInstance* GameInstance = Cast<UGameInfoInstance>(GetGameInstance());
	if (GameInstance && !GameInstance->IsCloudSaveEnabled())
	{
		return false;
	}

#if PLATFORM_ANDROID || PLATFORM_IOS
	return true;
#elif !UE_BUILD_SHIPPING
	return GameInstance && GameInstance->ShouldSimulateCloudSave();
#else
	return false;
#endif
}

void UCloudSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Collection.InitializeDependency<USaveSubsystem>();
	Collection.InitializeDependency<UEconomySubsystem>();
	Collection.InitializeDependency<UBallCatalogSubsystem>();
	Collection.InitializeDependency<UStoreSubsystem>();
	Collection.InitializeDependency<ULeaderboardSubsystem>();
	Super::Initialize(Collection);

	GActiveCloudSave = this;

	if (!IsSupported())
	{
		UE_LOG(LogFlappyHoop, Log, TEXT("[CloudSave] Not available on this platform (or turned off on BP_GameInfoInstance)"));
		return;
	}

	USaveSubsystem* Save = GetGameInstance()->GetSubsystem<USaveSubsystem>();
	if (Save)
	{
		Save->OnProgressionSaved.AddUObject(this, &UCloudSaveSubsystem::HandleProgressionSaved);
	}

	if (ULeaderboardSubsystem* Leaderboards = GetGameInstance()->GetSubsystem<ULeaderboardSubsystem>())
	{
		Leaderboards->OnSignInChanged.AddUObject(this, &UCloudSaveSubsystem::HandleSignInChanged);
		Leaderboards->OnSignInAttemptFinished.AddUObject(this, &UCloudSaveSubsystem::HandleSignInAttemptFinished);
	}

	FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddUObject(this, &UCloudSaveSubsystem::HandleEnterBackground);

#if PLATFORM_IOS
	// iCloud pushes changes made on other devices (and the first download after a reinstall) through this notification.
	TWeakObjectPtr<UCloudSaveSubsystem> WeakThis(this);
	id Observer = [[NSNotificationCenter defaultCenter]
		addObserverForName:NSUbiquitousKeyValueStoreDidChangeExternallyNotification
		            object:[NSUbiquitousKeyValueStore defaultStore]
		             queue:nil
		        usingBlock:^(NSNotification* Note)
	{
		AsyncTask(ENamedThreads::GameThread, [WeakThis]()
		{
			if (UCloudSaveSubsystem* Self = WeakThis.Get())
			{
				Self->HandleExternalCloudChange();
			}
		});
	}];
	ICloudObserver = (void*)[Observer retain];
	[[NSUbiquitousKeyValueStore defaultStore] synchronize]; // starts the download from iCloud
#endif

	// First sync. Android waits for Google Play Games sign-in (HandleSignInChanged); elsewhere sync right away.
#if PLATFORM_ANDROID
	const ULeaderboardSubsystem* Leaderboards = GetGameInstance()->GetSubsystem<ULeaderboardSubsystem>();
	if (Leaderboards && Leaderboards->IsSignedIn())
	{
		ScheduleSync(0.0f);
	}
#else
	ScheduleSync(0.0f);
#endif
}

void UCloudSaveSubsystem::Deinitialize()
{
	if (SyncTimerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(SyncTimerHandle);
		SyncTimerHandle.Reset();
	}
	if (WatchdogHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(WatchdogHandle);
		WatchdogHandle.Reset();
	}

	FCoreDelegates::ApplicationWillEnterBackgroundDelegate.RemoveAll(this);

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (USaveSubsystem* Save = GameInstance->GetSubsystem<USaveSubsystem>())
		{
			Save->OnProgressionSaved.RemoveAll(this);
		}
		if (ULeaderboardSubsystem* Leaderboards = GameInstance->GetSubsystem<ULeaderboardSubsystem>())
		{
			Leaderboards->OnSignInChanged.RemoveAll(this);
			Leaderboards->OnSignInAttemptFinished.RemoveAll(this);
		}
	}

#if PLATFORM_IOS
	if (ICloudObserver)
	{
		id Observer = (id)ICloudObserver;
		[[NSNotificationCenter defaultCenter] removeObserver:Observer];
		[Observer release];
		ICloudObserver = nullptr;
	}
#endif

	if (bSyncInFlight)
	{
		BackendDiscard();
	}

	if (GActiveCloudSave.Get() == this)
	{
		GActiveCloudSave.Reset();
	}

	Super::Deinitialize();
}

FUserProgression UCloudSaveSubsystem::GetDefaultProgression() const
{
	const UGameInfoInstance* GameInstance = Cast<UGameInfoInstance>(GetGameInstance());
	const TSubclassOf<UMySaveGame> SaveClass = GameInstance ? GameInstance->GetSaveGameClass() : nullptr;
	const UMySaveGame* Defaults = SaveClass ? SaveClass->GetDefaultObject<UMySaveGame>() : GetDefault<UMySaveGame>();
	return Defaults ? Defaults->UserProgression : FUserProgression();
}

// ---------------------------------------------------------------------------------------------------------------------
// Triggers
// ---------------------------------------------------------------------------------------------------------------------

void UCloudSaveSubsystem::HandleProgressionSaved(const FUserProgression& Progression)
{
	if (bApplyingCloudData)
	{
		return;
	}

	const FString Fingerprint = CloudFingerprint(Progression);
	if (Fingerprint == LastSyncedFingerprint)
	{
		return; // only device settings changed (music, SFX, difficulty)
	}

	// Gems going up is usually a purchase: get it into the cloud quickly.
	const bool bGemGain = Progression.TotalGems > (Progression.bCloudSynced ? Progression.CloudBaseGems : 0);
	ScheduleSync(bGemGain ? SyncDelayAfterGemGain : SyncDelayAfterChange);
}

void UCloudSaveSubsystem::HandleSignInChanged(bool bSignedIn)
{
#if PLATFORM_ANDROID
	if (bSignedIn)
	{
		ScheduleSync(0.0f);
	}
#endif
}

void UCloudSaveSubsystem::HandleSignInAttemptFinished(bool bSignedIn)
{
#if PLATFORM_ANDROID
	// The player cancelled (or sign-in failed) while waiting for a restore: tell the UI now.
	if (!bSignedIn && bRestoreRequested && !bSyncInFlight)
	{
		if (SyncTimerHandle.IsValid())
		{
			FTSTicker::GetCoreTicker().RemoveTicker(SyncTimerHandle);
			SyncTimerHandle.Reset();
		}
		FinishSync(EFlappyCloudSyncResult::NotSignedIn);
	}
#endif
}

void UCloudSaveSubsystem::HandleEnterBackground()
{
	const USaveSubsystem* Save = GetGameInstance()->GetSubsystem<USaveSubsystem>();
	if (Save && CloudFingerprint(Save->GetProgression()) != LastSyncedFingerprint)
	{
		SyncNow();
	}
}

void UCloudSaveSubsystem::HandleExternalCloudChange()
{
	UE_LOG(LogFlappyHoop, Log, TEXT("[CloudSave] Cloud data changed on another device / first download; merging"));
	SyncNow();
}

void UCloudSaveSubsystem::ScheduleSync(float DelaySeconds)
{
	if (SyncTimerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(SyncTimerHandle);
	}
	SyncTimerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UCloudSaveSubsystem::OnSyncTimer), FMath::Max(0.0f, DelaySeconds));
}

bool UCloudSaveSubsystem::OnSyncTimer(float DeltaTime)
{
	SyncTimerHandle.Reset();
	SyncNow();
	return false; // one-shot
}

void UCloudSaveSubsystem::SyncNow()
{
	if (!IsSupported())
	{
		FinishSync(EFlappyCloudSyncResult::Unsupported);
		return;
	}

	if (bSyncInFlight)
	{
		bSyncQueued = true;
		return;
	}

	StartSync();
}

void UCloudSaveSubsystem::RestoreNow()
{
	if (!IsSupported())
	{
		OnSyncFinished.Broadcast(EFlappyCloudSyncResult::Unsupported);
		return;
	}

	bRestoreRequested = true;

#if PLATFORM_ANDROID
	ULeaderboardSubsystem* Leaderboards = GetGameInstance()->GetSubsystem<ULeaderboardSubsystem>();
	if (Leaderboards && !Leaderboards->IsSignedIn())
	{
		// Signing in shows Google's sign-in UI if needed; a successful sign-in starts the sync.
		UE_LOG(LogFlappyHoop, Log, TEXT("[CloudSave] Restore: signing in to Google Play Games first"));
		Leaderboards->SignIn();
		ScheduleSync(SyncTimeoutSeconds); // fallback: report "not signed in" if sign-in never answers
		return;
	}
#endif

	SyncNow();
}

// ---------------------------------------------------------------------------------------------------------------------
// Sync
// ---------------------------------------------------------------------------------------------------------------------

void UCloudSaveSubsystem::StartSync()
{
#if PLATFORM_ANDROID
	const ULeaderboardSubsystem* Leaderboards = GetGameInstance()->GetSubsystem<ULeaderboardSubsystem>();
	if (!Leaderboards || !Leaderboards->IsSignedIn())
	{
		FinishSync(EFlappyCloudSyncResult::NotSignedIn);
		return;
	}
#endif

	bSyncInFlight = true;
	bSyncQueued = false;
	bHasPendingCommit = false;
	bNoCloudAccount = false;
	const int64 RequestId = ++CurrentRequestId;

	WatchdogHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UCloudSaveSubsystem::OnWatchdog), SyncTimeoutSeconds);

	UE_LOG(LogFlappyHoop, Log, TEXT("[CloudSave] Sync #%lld started"), RequestId);
	BackendLoad(RequestId);
}

bool UCloudSaveSubsystem::OnWatchdog(float DeltaTime)
{
	WatchdogHandle.Reset();
	if (bSyncInFlight)
	{
		UE_LOG(LogFlappyHoop, Warning, TEXT("[CloudSave] Sync #%lld timed out"), CurrentRequestId);
		++CurrentRequestId; // ignore the late answer
		BackendDiscard();
		FinishSync(EFlappyCloudSyncResult::Failed);
	}
	return false;
}

void UCloudSaveSubsystem::HandleCloudLoaded(int64 RequestId, bool bSuccess, const FString& Json)
{
	if (!bSyncInFlight || RequestId != CurrentRequestId)
	{
		return;
	}

	if (!bSuccess)
	{
		UE_LOG(LogFlappyHoop, Warning, TEXT("[CloudSave] Reading the cloud save failed"));
		BackendDiscard();
		FinishSync(EFlappyCloudSyncResult::Failed);
		return;
	}

	FFlappyCloudData Cloud;
	const bool bHasCloud = FFlappyCloudData::FromJson(Json, Cloud);
	if (!bHasCloud && !Json.IsEmpty())
	{
		UE_LOG(LogFlappyHoop, Warning, TEXT("[CloudSave] Cloud save unreadable; it will be replaced by this device's progress"));
	}

	const FString Merged = MergeAndApply(bHasCloud ? &Cloud : nullptr);
	BackendCommit(RequestId, Merged);
}

void UCloudSaveSubsystem::HandleCloudCommitted(int64 RequestId, bool bSuccess)
{
	if (!bSyncInFlight || RequestId != CurrentRequestId)
	{
		return;
	}

	if (bSuccess)
	{
		MarkCommitted();
		FinishSync(bNoCloudAccount ? EFlappyCloudSyncResult::NoICloudAccount : EFlappyCloudSyncResult::Synced);
	}
	else
	{
		UE_LOG(LogFlappyHoop, Warning, TEXT("[CloudSave] Writing the cloud save failed; will retry on the next change"));
		LastSyncedFingerprint.Reset(); // force a retry next time
		FinishSync(EFlappyCloudSyncResult::Failed);
	}
}

FString UCloudSaveSubsystem::MergeAndApply(const FFlappyCloudData* Cloud)
{
	USaveSubsystem* Save = GetGameInstance()->GetSubsystem<USaveSubsystem>();
	if (!Save)
	{
		return FString();
	}

	const FUserProgression Local = Save->GetProgression();
	FUserProgression Merged;
	Merge(Local, Cloud, GetDefaultProgression(), Merged, PendingCommit);
	bHasPendingCommit = true;

	const bool bGameplayChanged = CloudFingerprint(Local) != CloudFingerprint(Merged);
	const bool bAdsNewlyRemoved = !Local.bNoAds && Merged.bNoAds;

	bApplyingCloudData = true;
	Save->Save(Merged); // also stores the new cloud baseline
	bApplyingCloudData = false;

	LastSyncedFingerprint = CloudFingerprint(Merged);

	if (bGameplayChanged)
	{
		UE_LOG(LogFlappyHoop, Log, TEXT("[CloudSave] Applied: coins %d -> %d, gems %d -> %d, balls %d -> %d"),
			Local.TotalCoins, Merged.TotalCoins, Local.TotalGems, Merged.TotalGems, Local.OwnedBalls.Num(), Merged.OwnedBalls.Num());

		UGameInstance* GameInstance = GetGameInstance();
		if (UBallCatalogSubsystem* Catalog = GameInstance->GetSubsystem<UBallCatalogSubsystem>())
		{
			Catalog->RefreshFromSave(); // re-validates the selection, broadcasts OnBallsChanged
		}
		if (UEconomySubsystem* Economy = GameInstance->GetSubsystem<UEconomySubsystem>())
		{
			Economy->BroadcastBalance();
		}
		if (bAdsNewlyRemoved)
		{
			if (UStoreSubsystem* Store = GameInstance->GetSubsystem<UStoreSubsystem>())
			{
				Store->OnAdsRemoved.Broadcast();
			}
		}
		OnCloudProgressApplied.Broadcast();
	}

	return PendingCommit.ToJson();
}

void UCloudSaveSubsystem::MarkCommitted()
{
	USaveSubsystem* Save = GetGameInstance()->GetSubsystem<USaveSubsystem>();
	if (!Save || !bHasPendingCommit)
	{
		return;
	}

	// Only the baseline moves; anything the player did while the write was running stays a local change.
	FUserProgression Progression = Save->GetProgression();
	Progression.CloudBaseCoins = PendingCommit.Coins;
	Progression.CloudBaseGems = PendingCommit.Gems;
	Progression.bCloudSynced = true;

	bApplyingCloudData = true;
	Save->Save(Progression);
	bApplyingCloudData = false;

	bHasPendingCommit = false;
	UE_LOG(LogFlappyHoop, Log, TEXT("[CloudSave] Sync #%lld done (coins %d, gems %d, %d balls)"),
		CurrentRequestId, PendingCommit.Coins, PendingCommit.Gems, PendingCommit.OwnedBalls.Num());
}

void UCloudSaveSubsystem::FinishSync(EFlappyCloudSyncResult Result)
{
	if (WatchdogHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(WatchdogHandle);
		WatchdogHandle.Reset();
	}

	bSyncInFlight = false;
	bRestoreRequested = false;
	LastResult = Result;
	OnSyncFinished.Broadcast(Result);

	// Something changed locally while we were syncing: go again.
	const USaveSubsystem* Save = GetGameInstance() ? GetGameInstance()->GetSubsystem<USaveSubsystem>() : nullptr;
	const bool bLocalChangedSince = Result == EFlappyCloudSyncResult::Synced && Save
		&& CloudFingerprint(Save->GetProgression()) != LastSyncedFingerprint;
	if (bSyncQueued || bLocalChangedSince)
	{
		bSyncQueued = false;
		ScheduleSync(SyncDelayAfterGemGain);
	}
}

// =====================================================================================================================
// Backends
// =====================================================================================================================

#if PLATFORM_ANDROID

// --- Android: Google Play Games Saved Games (Java side: FlappyHoop_UPL.xml, added to GameActivity) ---

void UCloudSaveSubsystem::BackendLoad(int64 RequestId)
{
	JNIEnv* Env = FAndroidApplication::GetJavaEnv();
	static jmethodID OpenMethod = Env ? FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID,
		"AndroidThunkJava_FlappyCloudOpen", "(Ljava/lang/String;J)V", false) : nullptr;
	if (!Env || !OpenMethod)
	{
		UE_LOG(LogFlappyHoop, Warning, TEXT("[CloudSave] Java method AndroidThunkJava_FlappyCloudOpen missing (FlappyHoop_UPL.xml not applied?)"));
		HandleCloudLoaded(RequestId, false, FString());
		return;
	}

	auto Name = FJavaHelper::ToJavaString(Env, CloudSaveName);
	FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, OpenMethod, *Name, static_cast<jlong>(RequestId));
}

void UCloudSaveSubsystem::BackendCommit(int64 RequestId, const FString& Json)
{
	JNIEnv* Env = FAndroidApplication::GetJavaEnv();
	static jmethodID CommitMethod = Env ? FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID,
		"AndroidThunkJava_FlappyCloudCommit", "([BJ)V", false) : nullptr;
	if (!Env || !CommitMethod)
	{
		HandleCloudCommitted(RequestId, false);
		return;
	}

	const FTCHARToUTF8 Utf8(*Json);
	jbyteArray Bytes = Env->NewByteArray(Utf8.Length());
	Env->SetByteArrayRegion(Bytes, 0, Utf8.Length(), reinterpret_cast<const jbyte*>(Utf8.Get()));
	FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, CommitMethod, Bytes, static_cast<jlong>(RequestId));
	Env->DeleteLocalRef(Bytes);
}

void UCloudSaveSubsystem::BackendDiscard()
{
	JNIEnv* Env = FAndroidApplication::GetJavaEnv();
	static jmethodID DiscardMethod = Env ? FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID,
		"AndroidThunkJava_FlappyCloudDiscard", "()V", false) : nullptr;
	if (Env && DiscardMethod)
	{
		FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, DiscardMethod);
	}
}

/** Java -> C++: the snapshot was opened (bSuccess) and read. Data is UTF-8 JSON, empty for a new snapshot. */
JNI_METHOD void Java_com_epicgames_unreal_GameActivity_nativeFlappyCloudLoaded(JNIEnv* Env, jobject Thiz, jlong RequestId, jboolean bSuccess, jbyteArray Data)
{
	FString Json;
	if (Data)
	{
		const jsize Length = Env->GetArrayLength(Data);
		if (Length > 0)
		{
			TArray<uint8> Buffer;
			Buffer.SetNumUninitialized(Length);
			Env->GetByteArrayRegion(Data, 0, Length, reinterpret_cast<jbyte*>(Buffer.GetData()));
			const FUTF8ToTCHAR Converted(reinterpret_cast<const ANSICHAR*>(Buffer.GetData()), Buffer.Num());
			Json = FString::ConstructFromPtrSize(Converted.Get(), Converted.Length());
		}
	}

	const bool bOk = bSuccess == JNI_TRUE;
	const int64 Id = static_cast<int64>(RequestId);
	AsyncTask(ENamedThreads::GameThread, [Id, bOk, Json = MoveTemp(Json)]()
	{
		if (UCloudSaveSubsystem* CloudSave = GActiveCloudSave.Get())
		{
			CloudSave->HandleCloudLoaded(Id, bOk, Json);
		}
	});
}

/** Java -> C++: commitAndClose finished. */
JNI_METHOD void Java_com_epicgames_unreal_GameActivity_nativeFlappyCloudCommitted(JNIEnv* Env, jobject Thiz, jlong RequestId, jboolean bSuccess)
{
	const bool bOk = bSuccess == JNI_TRUE;
	const int64 Id = static_cast<int64>(RequestId);
	AsyncTask(ENamedThreads::GameThread, [Id, bOk]()
	{
		if (UCloudSaveSubsystem* CloudSave = GActiveCloudSave.Get())
		{
			CloudSave->HandleCloudCommitted(Id, bOk);
		}
	});
}

#elif PLATFORM_IOS

// --- iOS: iCloud key-value storage (synchronous; iCloud uploads in the background) ---

void UCloudSaveSubsystem::BackendLoad(int64 RequestId)
{
	// Without an iCloud account (or with iCloud Drive off) the key-value store still works locally and uploads
	// once iCloud is available, so sync anyway; the token only decides what the Restore button reports.
	bNoCloudAccount = [[NSFileManager defaultManager] ubiquityIdentityToken] == nil;

	NSUbiquitousKeyValueStore* Store = [NSUbiquitousKeyValueStore defaultStore];
	NSData* Data = [Store dataForKey:FString(CloudSaveName).GetNSString()];

	FString Json;
	if (Data && Data.length > 0)
	{
		const FUTF8ToTCHAR Converted(reinterpret_cast<const ANSICHAR*>(Data.bytes), (int32)Data.length);
		Json = FString::ConstructFromPtrSize(Converted.Get(), Converted.Length());
	}
	HandleCloudLoaded(RequestId, true, Json);
}

void UCloudSaveSubsystem::BackendCommit(int64 RequestId, const FString& Json)
{
	const FTCHARToUTF8 Utf8(*Json);
	NSData* Data = [NSData dataWithBytes:Utf8.Get() length:Utf8.Length()];

	NSUbiquitousKeyValueStore* Store = [NSUbiquitousKeyValueStore defaultStore];
	[Store setData:Data forKey:FString(CloudSaveName).GetNSString()];
	const bool bOk = [Store synchronize]; // false only when the iCloud entitlement is missing
	if (!bOk)
	{
		UE_LOG(LogFlappyHoop, Warning, TEXT("[CloudSave] iCloud synchronize failed: enable 'Enable Cloud Kit Support' in Project Settings > iOS and iCloud on the App ID"));
	}
	HandleCloudCommitted(RequestId, bOk);
}

void UCloudSaveSubsystem::BackendDiscard()
{
}

#else

// --- Editor/desktop: a JSON file stands in for the cloud (non-Shipping, bSimulateCloudSave on BP_GameInfoInstance) ---

void UCloudSaveSubsystem::BackendLoad(int64 RequestId)
{
#if !UE_BUILD_SHIPPING
	FString Json;
	FFileHelper::LoadFileToString(Json, *SimulatedCloudFile()); // missing file = empty cloud
	HandleCloudLoaded(RequestId, true, Json);
#else
	HandleCloudLoaded(RequestId, false, FString());
#endif
}

void UCloudSaveSubsystem::BackendCommit(int64 RequestId, const FString& Json)
{
#if !UE_BUILD_SHIPPING
	const bool bOk = FFileHelper::SaveStringToFile(Json, *SimulatedCloudFile(), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	HandleCloudCommitted(RequestId, bOk);
#else
	HandleCloudCommitted(RequestId, false);
#endif
}

void UCloudSaveSubsystem::BackendDiscard()
{
}

#endif
