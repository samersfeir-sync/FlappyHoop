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
		SaveGame = Cast<UMySaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("UserProgressionSlot"), 0));

	else
		SaveGame = Cast<UMySaveGame>(
			UGameplayStatics::CreateSaveGameObject(SaveGameClass));

	if (SaveGame)
	{
		UserProgression = SaveGame->UserProgression;
	}
}

FUserProgression& UGameInfoInstance::GetUserProgression()
{
	return UserProgression;
}

void UGameInfoInstance::Init()
{
	Super::Init();

	LoadUserProgression();
	InitializeADUnits();
}

void UGameInfoInstance::InitializeADUnits()
{
	#if PLATFORM_ANDROID
	BannerAdUnitID = TEXT("ca-app-pub-5279001044325653/5546944296");
	InterstitialAdUnitID = TEXT("ca-app-pub-5279001044325653/4163109237");
	RewardedAdUnitID = TEXT("ca-app-pub-5279001044325653/7632947939");
	
	#elif PLATFORM_IOS
	BannerAdUnitID = TEXT("ca-app-pub-xxxxxxxxxxxxxxxx/iOSBannerID");
	InterstitialAdUnitID = TEXT("ca-app-pub-xxxxxxxxxxxxxxxx/iOSInterstitialID");
	RewardedAdUnitID = TEXT("ca-app-pub-xxxxxxxxxxxxxxxx/iOSRewardedID");

	#else
	UE_LOG(LogTemp, Warning, TEXT("AdMob not initialized: Unsupported platform"))

	#endif
}
