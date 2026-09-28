// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FlappyAudioSubsystem.generated.h"

class UAudioComponent;

/** Background music that persists across level loads. The track is set on BP_GameInfoInstance (GameMusic). */
UCLASS()
class FLAPPYHOOP_API UFlappyAudioSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	static UFlappyAudioSubsystem* Get(const UObject* WorldContextObject);

	virtual void Deinitialize() override;

	/** Starts the music if it isn't already playing. */
	void PlayMusic();
	void StopMusic();

private:

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> MusicComponent = nullptr;
};
