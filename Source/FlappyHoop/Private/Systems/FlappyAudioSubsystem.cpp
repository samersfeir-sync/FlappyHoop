// Fill out your copyright notice in the Description page of Project Settings.

#include "Systems/FlappyAudioSubsystem.h"
#include "Framework/GameInfoInstance.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

UFlappyAudioSubsystem* UFlappyAudioSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UFlappyAudioSubsystem>() : nullptr;
}

void UFlappyAudioSubsystem::Deinitialize()
{
	StopMusic();
	Super::Deinitialize();
}

void UFlappyAudioSubsystem::PlayMusic()
{
	UGameInfoInstance* GameInstance = Cast<UGameInfoInstance>(GetGameInstance());
	USoundBase* Music = GameInstance ? GameInstance->GetGameMusic() : nullptr;
	if (!Music)
	{
		return;
	}

	if (!MusicComponent)
	{
		// Needs a world, so this is created lazily (after startup), and persists across map loads.
		MusicComponent = UGameplayStatics::CreateSound2D(GameInstance, Music, 1.0f, 1.0f, 0.0f, nullptr,
			/*bPersistAcrossLevelTransition*/ true, /*bAutoDestroy*/ false);

		if (MusicComponent)
		{
			MusicComponent->bIsUISound = true;
		}
	}

	if (MusicComponent && !MusicComponent->IsPlaying())
	{
		MusicComponent->Play();
	}
}

void UFlappyAudioSubsystem::StopMusic()
{
	if (MusicComponent)
	{
		MusicComponent->Stop();
	}
}
