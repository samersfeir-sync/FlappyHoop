// Fill out your copyright notice in the Description page of Project Settings.

#include "Systems/LeaderboardSubsystem.h"
#include "Core/FlappyHoop.h"
#include "Framework/GameInfoInstance.h"
#include "Systems/SaveSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

#if PLATFORM_ANDROID
#include "OnlineSubsystem.h"
#include "OnlineStats.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlineLeaderboardInterface.h"
#include "Interfaces/OnlineExternalUIInterface.h"
#endif

#if PLATFORM_IOS
#include "Async/Async.h"
#import <UIKit/UIKit.h>
#import <GameKit/GameKit.h>
#endif

namespace
{
#if PLATFORM_ANDROID
	const FName GooglePlaySubsystemName(TEXT("GOOGLEPLAY"));

	IOnlineSubsystem* GetGooglePlay()
	{
		return IOnlineSubsystem::Get(GooglePlaySubsystemName);
	}
#endif

#if PLATFORM_IOS
	/** Sign-in view controller handed over by Game Center (retained), shown when the player asks for it. */
	UIViewController* GGameCenterSignInViewController = nil;

	UIViewController* GetRootViewController()
	{
		for (UIScene* Scene in [UIApplication sharedApplication].connectedScenes)
		{
			if (![Scene isKindOfClass:[UIWindowScene class]])
			{
				continue;
			}
			for (UIWindow* Window in ((UIWindowScene*)Scene).windows)
			{
				if (Window.isKeyWindow)
				{
					return Window.rootViewController;
				}
			}
		}
		return nil;
	}

	/** Must run on the main thread. */
	void PresentGameCenterSignIn()
	{
		UIViewController* Root = GetRootViewController();
		if (GGameCenterSignInViewController && Root && Root.presentedViewController == nil)
		{
			[Root presentViewController:GGameCenterSignInViewController animated:YES completion:nil];
		}
	}
#endif
}

ULeaderboardSubsystem* ULeaderboardSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<ULeaderboardSubsystem>() : nullptr;
}

bool ULeaderboardSubsystem::IsSupported()
{
#if PLATFORM_ANDROID || PLATFORM_IOS
	return true;
#else
	return false;
#endif
}

void ULeaderboardSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Collection.InitializeDependency<USaveSubsystem>();
	Super::Initialize(Collection);

#if PLATFORM_IOS
	// Game Center calls the handler on launch and whenever the app returns to the foreground.
	InstallGameCenterAuthHandler();
#endif

	const UGameInfoInstance* GameInstance = Cast<UGameInfoInstance>(GetGameInstance());
	if (!GameInstance || GameInstance->ShouldSignInToGameServicesAtStartup())
	{
		SignIn();
	}
}

void ULeaderboardSubsystem::Deinitialize()
{
#if PLATFORM_ANDROID
	if (IOnlineSubsystem* GooglePlay = GetGooglePlay())
	{
		if (IOnlineIdentityPtr Identity = GooglePlay->GetIdentityInterface())
		{
			Identity->ClearOnLoginCompleteDelegate_Handle(0, LoginCompleteHandle);
		}
	}
#endif

	Super::Deinitialize();
}

void ULeaderboardSubsystem::SetSignedIn(bool bNewSignedIn)
{
	if (bSignedIn == bNewSignedIn)
	{
		return;
	}

	bSignedIn = bNewSignedIn;
	UE_LOG(LogFlappyHoop, Log, TEXT("[Leaderboard] Signed %s"), bSignedIn ? TEXT("in") : TEXT("out"));
	OnSignInChanged.Broadcast(bSignedIn);

	if (bSignedIn)
	{
		SubmitPendingScores();
	}
}

void ULeaderboardSubsystem::SubmitPendingScores()
{
	// The saved bests may never have reached the service (played offline / before signing in).
	int32 BestEasy = PendingScoreEasy;
	int32 BestHard = PendingScoreHard;
	if (const USaveSubsystem* Save = GetGameInstance()->GetSubsystem<USaveSubsystem>())
	{
		BestEasy = FMath::Max(BestEasy, Save->GetProgression().HighScore);
		BestHard = FMath::Max(BestHard, Save->GetProgression().HighScoreHard);
	}
	PendingScoreEasy = 0;
	PendingScoreHard = 0;

	if (BestEasy > 0)
	{
		SubmitScore(BestEasy, EDifficultyMode::Easy);
	}
	if (BestHard > 0)
	{
		SubmitScore(BestHard, EDifficultyMode::Hard);
	}
}

void ULeaderboardSubsystem::SignIn()
{
#if PLATFORM_ANDROID
	if (bSignedIn || bSignInInProgress)
	{
		return;
	}

	IOnlineSubsystem* GooglePlay = GetGooglePlay();
	IOnlineIdentityPtr Identity = GooglePlay ? GooglePlay->GetIdentityInterface() : nullptr;
	if (!Identity)
	{
		UE_LOG(LogFlappyHoop, Warning, TEXT("[Leaderboard] Google Play Games unavailable (enable Google Play support + Games App ID in Project Settings > Android)"));
		return;
	}

	if (!LoginCompleteHandle.IsValid())
	{
		LoginCompleteHandle = Identity->AddOnLoginCompleteDelegate_Handle(0,
			FOnLoginCompleteDelegate::CreateUObject(this, &ULeaderboardSubsystem::OnGooglePlayLoginComplete));
	}

	bSignInInProgress = true;
	Identity->Login(0, FOnlineAccountCredentials()); // already signed in: completes synchronously
#elif PLATFORM_IOS
	// Game Center signs in by itself; if it handed us a sign-in screen, show it now.
	dispatch_async(dispatch_get_main_queue(), ^{ PresentGameCenterSignIn(); });
#endif
}

void ULeaderboardSubsystem::SubmitScore(int32 Score, EDifficultyMode Mode)
{
	if (Score <= 0)
	{
		return;
	}

	if (!bSignedIn)
	{
		int32& Pending = PendingScoreFor(Mode);
		Pending = FMath::Max(Pending, Score);
		return;
	}

#if PLATFORM_ANDROID
	SubmitScoreGooglePlay(Score, Mode);
#elif PLATFORM_IOS
	SubmitScoreGameCenter(Score, Mode);
#endif
}

void ULeaderboardSubsystem::ShowLeaderboard(EDifficultyMode Mode)
{
	// Editor / desktop: no game service, the button does nothing.
	if (!IsSupported())
	{
		return;
	}

	if (!bSignedIn)
	{
		bShowLeaderboardAfterSignIn = true;
		ShowLeaderboardMode = Mode;
#if PLATFORM_IOS
		if (!bGameCenterHasSignInUI)
		{
			// No sign-in screen available: the player turned Game Center off (or cancelled it too often).
			UE_LOG(LogFlappyHoop, Warning, TEXT("[Leaderboard] Game Center is not signed in; the player can sign in from Settings > Game Center"));
			bShowLeaderboardAfterSignIn = false;
		}
#endif
		SignIn();
		return;
	}

#if PLATFORM_ANDROID
	IOnlineSubsystem* GooglePlay = GetGooglePlay();
	if (IOnlineExternalUIPtr ExternalUI = GooglePlay ? GooglePlay->GetExternalUIInterface() : nullptr)
	{
		ExternalUI->ShowLeaderboardUI(FlappyHoopLeaderboards::ForMode(Mode));
	}
#elif PLATFORM_IOS
	const FString LeaderboardId = GetGameCenterLeaderboardId(Mode);
	if (LeaderboardId.IsEmpty())
	{
		UE_LOG(LogFlappyHoop, Warning, TEXT("[Leaderboard] Game Center leaderboard ID for this difficulty is empty on BP_GameInfoInstance"));
		return;
	}

	// Engine helper (Core, iOS): presents GKGameCenterViewController for this leaderboard and dismisses it.
	extern CORE_API void IOSShowLeaderboardUI(const FString& CategoryName);
	IOSShowLeaderboardUI(LeaderboardId);
#endif
}

FString ULeaderboardSubsystem::GetGameCenterLeaderboardId(EDifficultyMode Mode) const
{
	const UGameInfoInstance* GameInstance = Cast<UGameInfoInstance>(GetGameInstance());
	if (!GameInstance)
	{
		return FString();
	}
	return Mode == EDifficultyMode::Hard ? GameInstance->GetGameCenterHardLeaderboardID() : GameInstance->GetGameCenterLeaderboardID();
}

// ---------------------------------------------------------------------------------------------------------------------
// Android: Google Play Games Services (v2) through OnlineSubsystemGooglePlay
// ---------------------------------------------------------------------------------------------------------------------

void ULeaderboardSubsystem::OnGooglePlayLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
	bSignInInProgress = false;

	if (!bWasSuccessful)
	{
		UE_LOG(LogFlappyHoop, Log, TEXT("[Leaderboard] Google Play Games sign-in failed: %s"), *Error);
		bShowLeaderboardAfterSignIn = false;
		return;
	}

	SetSignedIn(true);

	if (bShowLeaderboardAfterSignIn)
	{
		bShowLeaderboardAfterSignIn = false;
		ShowLeaderboard(ShowLeaderboardMode);
	}
}

void ULeaderboardSubsystem::SubmitScoreGooglePlay(int32 Score, EDifficultyMode Mode)
{
#if PLATFORM_ANDROID
	IOnlineSubsystem* GooglePlay = GetGooglePlay();
	IOnlineIdentityPtr Identity = GooglePlay ? GooglePlay->GetIdentityInterface() : nullptr;
	IOnlineLeaderboardsPtr Leaderboards = GooglePlay ? GooglePlay->GetLeaderboardsInterface() : nullptr;
	FUniqueNetIdPtr UserId = Identity ? Identity->GetUniquePlayerId(0) : nullptr;
	if (!Leaderboards || !UserId.IsValid())
	{
		int32& Pending = PendingScoreFor(Mode);
		Pending = FMath::Max(Pending, Score);
		return;
	}

	const TCHAR* LeaderboardName = FlappyHoopLeaderboards::ForMode(Mode);
	FOnlineLeaderboardWrite Write;
	Write.LeaderboardNames.Add(LeaderboardName);
	Write.SetIntStat(TEXT("Score"), Score);

	if (Leaderboards->WriteLeaderboards(NAME_GameSession, *UserId, Write))
	{
		Leaderboards->FlushLeaderboards(NAME_GameSession);
		UE_LOG(LogFlappyHoop, Log, TEXT("[Leaderboard] Submitted %d to Google Play (%s)"), Score, LeaderboardName);
	}
	else
	{
		UE_LOG(LogFlappyHoop, Warning, TEXT("[Leaderboard] WriteLeaderboards failed (is '%s' in the Android Leaderboard Map?)"), LeaderboardName);
	}
#endif
}

// ---------------------------------------------------------------------------------------------------------------------
// iOS: Game Center (GameKit)
// ---------------------------------------------------------------------------------------------------------------------

void ULeaderboardSubsystem::InstallGameCenterAuthHandler()
{
#if PLATFORM_IOS
	TWeakObjectPtr<ULeaderboardSubsystem> WeakThis(this);

	dispatch_async(dispatch_get_main_queue(), ^{
		[GKLocalPlayer localPlayer].authenticateHandler = ^(UIViewController* SignInViewController, NSError* Error)
		{
			// Runs on the main thread.
			if (GGameCenterSignInViewController != SignInViewController)
			{
				[GGameCenterSignInViewController release];
				GGameCenterSignInViewController = [SignInViewController retain];
			}

			const bool bAuthenticated = [GKLocalPlayer localPlayer].isAuthenticated;
			const bool bHasSignInUI = SignInViewController != nil;

			if (Error && !bAuthenticated)
			{
				UE_LOG(LogFlappyHoop, Log, TEXT("[Leaderboard] Game Center: %s"), *FString([Error localizedDescription]));
			}

			AsyncTask(ENamedThreads::GameThread, [WeakThis, bAuthenticated, bHasSignInUI]()
			{
				if (ULeaderboardSubsystem* Self = WeakThis.Get())
				{
					Self->HandleGameCenterAuth(bAuthenticated, bHasSignInUI);
				}
			});
		};
	});
#endif
}

void ULeaderboardSubsystem::HandleGameCenterAuth(bool bAuthenticated, bool bHasSignInUI)
{
	bGameCenterHasSignInUI = bHasSignInUI;
	SetSignedIn(bAuthenticated);

	if (bAuthenticated && bShowLeaderboardAfterSignIn)
	{
		bShowLeaderboardAfterSignIn = false;
		ShowLeaderboard(ShowLeaderboardMode);
		return;
	}

#if PLATFORM_IOS
	// A sign-in screen arrived: show it now if the player is waiting for it, or at startup when configured.
	if (!bAuthenticated && bHasSignInUI)
	{
		const UGameInfoInstance* GameInstance = Cast<UGameInfoInstance>(GetGameInstance());
		const bool bAtStartup = !GameInstance || GameInstance->ShouldSignInToGameServicesAtStartup();
		if (bShowLeaderboardAfterSignIn || bAtStartup)
		{
			dispatch_async(dispatch_get_main_queue(), ^{ PresentGameCenterSignIn(); });
		}
	}
#endif
}

void ULeaderboardSubsystem::SubmitScoreGameCenter(int32 Score, EDifficultyMode Mode)
{
#if PLATFORM_IOS
	const FString LeaderboardId = GetGameCenterLeaderboardId(Mode);
	if (LeaderboardId.IsEmpty())
	{
		UE_LOG(LogFlappyHoop, Warning, TEXT("[Leaderboard] Game Center leaderboard ID for this difficulty is empty on BP_GameInfoInstance"));
		return;
	}

	NSString* Id = [LeaderboardId.GetNSString() retain];
	const int32 ScoreCopy = Score;

	dispatch_async(dispatch_get_main_queue(), ^{
		[GKLeaderboard submitScore:ScoreCopy
		                   context:0
		                    player:[GKLocalPlayer localPlayer]
		            leaderboardIDs:@[Id]
		         completionHandler:^(NSError* Error)
		{
			if (Error)
			{
				UE_LOG(LogFlappyHoop, Warning, TEXT("[Leaderboard] Game Center submit failed: %s"), *FString([Error localizedDescription]));
			}
			else
			{
				UE_LOG(LogFlappyHoop, Log, TEXT("[Leaderboard] Submitted %d to Game Center"), ScoreCopy);
			}
		}];
		[Id release];
	});
#endif
}
