// Fill out your copyright notice in the Description page of Project Settings.


#include "GameInfoInstance.h"
#include "Kismet/GameplayStatics.h"
#include "MySaveGame.h"

void UGameInfoInstance::SaveUserProgression(FUserProgression& NewUserProgression)
{
	if (SaveGame)
	{
		UserProgression = NewUserProgression;
		SaveGame->UserProgression = NewUserProgression;
		UGameplayStatics::SaveGameToSlot(SaveGame, "UserProgressionSlot", 0);
	}
}

void UGameInfoInstance::LoadUserProgression()
{
	if (UGameplayStatics::DoesSaveGameExist(TEXT("UserProgressionSlot"), 0))
	{
		SaveGame = Cast<UMySaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("UserProgressionSlot"), 0));

		if (SaveGame)
		{
			UserProgression = SaveGame->UserProgression;
		}
	}
	else
	{
		SaveGame = Cast<UMySaveGame>(
			UGameplayStatics::CreateSaveGameObject(UMySaveGame::StaticClass()));
	}
}

void UGameInfoInstance::Init()
{
	Super::Init();

	LoadUserProgression();
}
