// Fill out your copyright notice in the Description page of Project Settings.


#include "FunctionsLibrary.h"
#include "GameInstanceInterface.h"
#include "GameModeInterface.h"
#include "GameFramework/GameModeBase.h"
#include "Components/TextBlock.h"
#include "MGAndroidBillingResult.h"

IGameInstanceInterface* UFunctionsLibrary::GetGameInstanceInterface(UObject* WorldContextObject)
{
	if (!WorldContextObject) return nullptr;

	UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);
	if (!World) return nullptr;

	UGameInstance* GameInstance = World->GetGameInstance();
	if (!GameInstance) return nullptr;

	if (GameInstance->GetClass()->ImplementsInterface(UGameInstanceInterface::StaticClass()))
	{
		return Cast<IGameInstanceInterface>(GameInstance);
	}

	return nullptr;
}

IGameModeInterface* UFunctionsLibrary::GetGameModeInterface(UObject* WorldContextObject)
{
	if (!WorldContextObject) return nullptr;

	UWorld* World = GEngine->GetWorldFromContextObjectChecked(WorldContextObject);
	if (!World) return nullptr;

	AGameModeBase* GameMode = World->GetAuthGameMode();
	if (!GameMode) return nullptr;

	if (GameMode->GetClass()->ImplementsInterface(UGameModeInterface::StaticClass()))
	{
		return Cast<IGameModeInterface>(GameMode);
	}

	return nullptr;
}

void UFunctionsLibrary::SetHighScoreText(UTextBlock* TextBlock, int NewHighScore)
{
	if (TextBlock)
	{
		TextBlock->SetText(
			FText::FromString(FString::Printf(TEXT("Best: %d"), NewHighScore))
		);
	}
}

bool UFunctionsLibrary::BillingResponseOK(UMGAndroidBillingResult* BillingResult)
{
	bool bOk = BillingResult && BillingResult->GetResponseCode() == EMGAndroidBillingResponseCode::Ok;

	FString Message = FString::Printf(TEXT("BillingResponseOK: %s"), bOk ? TEXT("true") : TEXT("false"));
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, Message);

	return bOk;
}
