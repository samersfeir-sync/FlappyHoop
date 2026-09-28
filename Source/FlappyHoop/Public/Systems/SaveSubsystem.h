// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Data/UserProgression.h"
#include "SaveSubsystem.generated.h"

class UMySaveGame;

/**
 * Owns the player's progression and the save file.
 * Loads (and migrates) on startup, and writes changes asynchronously
 * (one write in flight at a time, the newest state always written last, flushed on shutdown).
 */
UCLASS()
class FLAPPYHOOP_API USaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	static USaveSubsystem* Get(const UObject* WorldContextObject);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	const FUserProgression& GetProgression() const { return Progression; }

	/** Replaces the progression and saves it to disk (async). */
	void Save(const FUserProgression& NewProgression);

private:

	static const FString SlotName;

	void Load();
	/** Converts old saves (full shop list) to the current format (owned ball types only). */
	void MigrateSave();

	void WriteToDisk();
	void OnAsyncSaveFinished(const FString& Slot, const int32 UserIndex, bool bSuccess);

	UPROPERTY(Transient)
	TObjectPtr<UMySaveGame> SaveGame = nullptr;

	FUserProgression Progression;

	bool bSaveInFlight = false;
	bool bSaveDirty = false;
};
