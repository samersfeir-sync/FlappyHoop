// Fill out your copyright notice in the Description page of Project Settings.


#include "GameInfoInstance.h"
#include "Kismet/GameplayStatics.h"
#include "MySaveGame.h"
#include "MoviePlayer.h"
#include "Interface/AGBannerAdInterface.h"
#include "Ads/AGAdLibrary.h"

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
	GetMoviePlayer()->OnMoviePlaybackFinished().AddUObject(this, &UGameInfoInstance::OnMoviePlaybackFinished);

	//banner ad
	BannerAdInterface = UAGAdLibrary::MakeBannerAd(
		BannerADUnitID,
		EAdSizeType::Banner,
		EAdPosition::Bottom
	);

	if (BannerAdInterface)
	{
		BannerAdInterface->LoadAd(false);
	}
}


void UGameInfoInstance::OnMoviePlaybackFinished()
{
	if (BannerAdInterface)
	{
		BannerAdInterface->Show();
	}
}

void UGameInfoInstance::InitializeADUnits()
{
#if PLATFORM_ANDROID
	BannerADUnitID = TEXT("ca-app-pub-5279001044325653/5546944296");
	InterstitialADUnitID = TEXT("ca-app-pub-5279001044325653/4163109237");
	RewardedADUnitID = TEXT("ca-app-pub-5279001044325653/7632947939");

#elif PLATFORM_IOS
	BannerADUnitID = TEXT("");
	InterstitialADUnitID = TEXT("");
	RewardedADUnitID = TEXT("");

#endif
}
