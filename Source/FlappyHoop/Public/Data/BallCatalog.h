// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/BallType.h"
#include "BallCatalog.generated.h"

class UStaticMesh;
class UTexture2D;
class UMySaveGame;
class ABaseGameMode;

/** Everything the game needs to know about one ball, in one row. */
USTRUCT(BlueprintType)
struct FBallCatalogEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball")
	EBallType BallType = EBallType::Basketball;

	/** Mesh used on the ball in the level. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball")
	TObjectPtr<UStaticMesh> Mesh = nullptr;

	/** Picture shown in the shop. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball")
	TObjectPtr<UTexture2D> Icon = nullptr;

	/** Coin price in the shop. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball", meta = (ClampMin = "0"))
	int32 Price = 200;

	/** Owned from the start (e.g. the basketball). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ball")
	bool bOwnedByDefault = false;
};

/**
 * The shop's ball list: mesh, shop icon, price and default ownership per ball. Shop order = array order.
 * Create one (Content Browser > Miscellaneous > Data Asset > BallCatalog) and assign it on BP_GameInfoInstance.
 * Adding a ball = adding a row here (plus an EBallType value); saves only store which types are owned.
 */
UCLASS(BlueprintType)
class FLAPPYHOOP_API UBallCatalog : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Balls", meta = (TitleProperty = "BallType"))
	TArray<FBallCatalogEntry> Balls;

	const FBallCatalogEntry* Find(EBallType BallType) const;

#if WITH_EDITORONLY_DATA
	/** Save-game Blueprint whose Balls Owned defaults hold the old icons/prices (e.g. BP_MySaveGame). */
	UPROPERTY(EditDefaultsOnly, Category = "Import From Old Data")
	TSubclassOf<UMySaveGame> ImportSaveGameClass;

	/** Game mode Blueprint whose Ball Meshes map holds the old meshes (e.g. BP_BaseGameMode). */
	UPROPERTY(EditDefaultsOnly, Category = "Import From Old Data")
	TSubclassOf<ABaseGameMode> ImportGameModeClass;
#endif

	/**
	 * Replaces the Balls list with the old data: icon, price and default ownership from the save-game
	 * Blueprint, meshes from the game mode Blueprint. Pick both classes above, click, then save the asset.
	 */
	UFUNCTION(CallInEditor, Category = "Import From Old Data")
	void ImportFromOldData();
};
