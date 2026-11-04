// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Data/DifficultyTypes.h"
#include "LeaderboardSubsystem.generated.h"

class FUniqueNetId;

/** Signed in to / out of Google Play Games or Game Center. */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnGameServicesSignInChanged, bool /*bSignedIn*/);

namespace FlappyHoopLeaderboards
{
	/**
	 * Logical name of the high-score leaderboard.
	 * Android: must match a "Leaderboard Map" entry (Project Settings > Android > Google Play Services).
	 * iOS: the Game Center ID is set on BP_GameInfoInstance (GameCenterLeaderboardID).
	 */
	inline const TCHAR* HighScore = TEXT("HighScore");

	/** Hard mode's leaderboard (Android Leaderboard Map entry; iOS: GameCenterHardLeaderboardID). */
	inline const TCHAR* HighScoreHard = TEXT("HighScoreHard");

	inline const TCHAR* ForMode(EDifficultyMode Mode) { return Mode == EDifficultyMode::Hard ? HighScoreHard : HighScore; }
}

/**
 * High-score leaderboard on Google Play Games Services (Android, via OnlineSubsystemGooglePlay / PGS v2)
 * and Game Center (iOS, GameKit directly: the engine's iOS leaderboard write path ignores the leaderboard ID).
 * Scores submitted while signed out are kept (best only) and sent after sign-in, together with the saved high score.
 * On other platforms everything is a no-op.
 */
UCLASS()
class FLAPPYHOOP_API ULeaderboardSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	static ULeaderboardSubsystem* Get(const UObject* WorldContextObject);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** True on platforms with a game service (Android, iOS). Use it to hide the leaderboard button elsewhere. */
	static bool IsSupported();

	bool IsSignedIn() const { return bSignedIn; }

	/** Starts sign-in (may show the platform's sign-in UI). Called at startup when bSignInToGameServicesAtStartup is on. */
	void SignIn();

	/** Reports a score to that difficulty's leaderboard (the services keep the best one). */
	void SubmitScore(int32 Score, EDifficultyMode Mode = EDifficultyMode::Easy);

	/** Opens the platform leaderboard UI for that difficulty, signing in first if needed. */
	void ShowLeaderboard(EDifficultyMode Mode = EDifficultyMode::Easy);

	FOnGameServicesSignInChanged OnSignInChanged;

	/** Fired whenever a sign-in attempt ends (success or failure/cancel), even if the state didn't change. */
	FOnGameServicesSignInChanged OnSignInAttemptFinished;

private:

	void SetSignedIn(bool bNewSignedIn);
	void SubmitPendingScores();
	FString GetGameCenterLeaderboardId(EDifficultyMode Mode) const;

	bool bSignedIn = false;
	bool bSignInInProgress = false;
	bool bShowLeaderboardAfterSignIn = false;
	EDifficultyMode ShowLeaderboardMode = EDifficultyMode::Easy;

	/** Best scores reported while signed out (0 = none). */
	int32 PendingScoreEasy = 0;
	int32 PendingScoreHard = 0;
	int32& PendingScoreFor(EDifficultyMode Mode) { return Mode == EDifficultyMode::Hard ? PendingScoreHard : PendingScoreEasy; }

	// Android (Google Play Games via OnlineSubsystemGooglePlay)
	void SubmitScoreGooglePlay(int32 Score, EDifficultyMode Mode);
	void OnGooglePlayLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error);
	FDelegateHandle LoginCompleteHandle;

	// iOS (Game Center via GameKit)
	void SubmitScoreGameCenter(int32 Score, EDifficultyMode Mode);
	void InstallGameCenterAuthHandler();
	bool bGameCenterHasSignInUI = false;

public:

	/** Called on the game thread by the Game Center authenticate handler (iOS only). */
	void HandleGameCenterAuth(bool bAuthenticated, bool bHasSignInUI);
};
