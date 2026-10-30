// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/GameInfoInstance.h"
#include "Data/MySaveGame.h"
#include "Systems/SaveSubsystem.h"
#include "Systems/AdsSubsystem.h"
#include "Systems/FlappyAudioSubsystem.h"
#include "Sound/SoundBase.h"
#include "MoviePlayer.h"
#include "HAL/IConsoleManager.h"
#include "Core/FlappyHoop.h"

void UGameInfoInstance::Init()
{
	// Initializes all game-instance subsystems (save loads here, store connects here).
	Super::Init();

	ApplyMobileResolution();

	if (IGameMoviePlayer* MoviePlayer = GetMoviePlayer())
	{
		MoviePlayer->OnMoviePlaybackFinished().AddUObject(this, &UGameInfoInstance::HandleStartupMovieFinished);
	}
}

void UGameInfoInstance::OnStart()
{
	Super::OnStart();

	// In PIE (and whenever no startup movie plays) OnMoviePlaybackFinished never fires.
	IGameMoviePlayer* MoviePlayer = GetMoviePlayer();
	if (!MoviePlayer || !MoviePlayer->IsMovieCurrentlyPlaying())
	{
		StartMusicAndBanner();
	}
}

void UGameInfoInstance::Shutdown()
{
	if (IGameMoviePlayer* MoviePlayer = GetMoviePlayer())
	{
		MoviePlayer->OnMoviePlaybackFinished().RemoveAll(this);
	}

	// Subsystems (save flush, billing disconnect, music stop) are deinitialized by Super.
	Super::Shutdown();
}

void UGameInfoInstance::ApplyMobileResolution()
{
#if PLATFORM_ANDROID || PLATFORM_IOS
	// Device profiles (Android_High, GPU-specific ones...) set this per phone; SetByCode ranks above them.
	if (IConsoleVariable* ContentScale = IConsoleManager::Get().FindConsoleVariable(TEXT("r.MobileContentScaleFactor")))
	{
		const float Before = ContentScale->GetFloat();
		ContentScale->Set(MobileResolutionScale, ECVF_SetByCode);
		UE_LOG(LogFlappyHoop, Log, TEXT("Display: r.MobileContentScaleFactor %.2f -> %.2f"), Before, ContentScale->GetFloat());
	}
	else
	{
		UE_LOG(LogFlappyHoop, Warning, TEXT("Display: r.MobileContentScaleFactor not found"));
	}
#endif
}

void UGameInfoInstance::HandleStartupMovieFinished()
{
	StartMusicAndBanner();
}

void UGameInfoInstance::StartMusicAndBanner()
{
	if (bStartupHandled)
	{
		return;
	}
	bStartupHandled = true;

	const USaveSubsystem* Save = GetSubsystem<USaveSubsystem>();
	if (Save && !Save->GetProgression().bIsMusicMuted)
	{
		if (UFlappyAudioSubsystem* Audio = GetSubsystem<UFlappyAudioSubsystem>())
		{
			Audio->PlayMusic();
		}
	}

	if (UAdsSubsystem* Ads = GetSubsystem<UAdsSubsystem>())
	{
		Ads->StartConsentFlow(); // Google's consent form, when required (EEA/UK/CH)
		Ads->ShowBanner();       // waits for consent; no-op when ads were removed
	}
}
