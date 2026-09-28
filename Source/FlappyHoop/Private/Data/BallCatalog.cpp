// Fill out your copyright notice in the Description page of Project Settings.

#include "Data/BallCatalog.h"
#include "Core/FlappyHoop.h"
#include "Data/MySaveGame.h"
#include "Framework/BaseGameMode.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"

const FBallCatalogEntry* UBallCatalog::Find(EBallType BallType) const
{
	return Balls.FindByPredicate([BallType](const FBallCatalogEntry& Entry) { return Entry.BallType == BallType; });
}

void UBallCatalog::ImportFromOldData()
{
#if WITH_EDITOR
	const UMySaveGame* SaveDefaults = ImportSaveGameClass ? ImportSaveGameClass->GetDefaultObject<UMySaveGame>() : nullptr;
	if (!SaveDefaults)
	{
		UE_LOG(LogFlappyHoop, Warning, TEXT("BallCatalog import: pick a save-game class (e.g. BP_MySaveGame) first."));
		return;
	}

	const ABaseGameMode* GameModeDefaults = ImportGameModeClass ? ImportGameModeClass->GetDefaultObject<ABaseGameMode>() : nullptr;
	if (!GameModeDefaults)
	{
		UE_LOG(LogFlappyHoop, Warning, TEXT("BallCatalog import: no game mode class picked, meshes will be left empty."));
	}

	Modify(); // undo support + marks the asset dirty

	TArray<FBallCatalogEntry> Imported;
	for (const FBallsShopStruct& Old : SaveDefaults->UserProgression.BallsOwned)
	{
		if (Imported.ContainsByPredicate([&Old](const FBallCatalogEntry& E) { return E.BallType == Old.BallType; }))
		{
			continue; // skip duplicate rows in the old list
		}

		FBallCatalogEntry Entry;
		Entry.BallType = Old.BallType;
		Entry.Icon = Old.BallTexture;
		Entry.Price = Old.Price;
		Entry.bOwnedByDefault = Old.IsPurchased;

		if (GameModeDefaults)
		{
			if (const TObjectPtr<UStaticMesh>* Mesh = GameModeDefaults->GetLegacyBallMeshes().Find(Old.BallType))
			{
				Entry.Mesh = *Mesh;
			}
		}

		Imported.Add(Entry);
	}

	// Make sure at least one ball is free, otherwise nothing could ever be selected.
	if (!Imported.IsEmpty() && !Imported.ContainsByPredicate([](const FBallCatalogEntry& E) { return E.bOwnedByDefault; }))
	{
		Imported[0].bOwnedByDefault = true;
	}

	Balls = MoveTemp(Imported);
	UE_LOG(LogFlappyHoop, Log, TEXT("BallCatalog import: %d balls imported. Save the asset."), Balls.Num());
#endif
}
