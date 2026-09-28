// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Data/BallCatalog.h"
#include "BallCatalogSubsystem.generated.h"

class USaveSubsystem;

/** Fired when a ball is unlocked or a different ball is selected. */
DECLARE_MULTICAST_DELEGATE(FOnBallsChanged);

/**
 * Which balls exist (from the UBallCatalog on BP_GameInfoInstance), which the player owns and which is selected.
 * Ownership and selection are stored in the save as ball types only.
 *
 * If no catalog is assigned yet, it falls back to the old data (the ball list on the save-game Blueprint's
 * defaults) so the game keeps working; meshes then come from the game mode's BallMeshes map.
 */
UCLASS()
class FLAPPYHOOP_API UBallCatalogSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	static UBallCatalogSubsystem* Get(const UObject* WorldContextObject);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** All balls, in shop order. */
	const TArray<FBallCatalogEntry>& GetBalls() const { return Balls; }
	const FBallCatalogEntry* Find(EBallType BallType) const;

	bool IsOwned(EBallType BallType) const;
	EBallType GetSelected() const;

	/** Marks the ball owned and selects it (payment is the caller's job). Returns false if unknown or already owned. */
	bool Unlock(EBallType BallType);

	/** Selects an owned ball. Returns false if it isn't owned. */
	bool Select(EBallType BallType);

	/** True while running on the old data because no catalog asset is assigned. */
	bool IsUsingLegacyData() const { return bLegacyData; }

	FOnBallsChanged OnBallsChanged;

private:

	USaveSubsystem* GetSave() const;

	void BuildBallList();

	/** Makes sure default balls are owned and the selected ball is an owned one. */
	void ValidateSave();

	UPROPERTY(Transient)
	TArray<FBallCatalogEntry> Balls;
	bool bLegacyData = false;
};
