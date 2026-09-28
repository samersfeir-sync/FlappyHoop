// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/FunctionsLibrary.h"
#include "Core/FlappyHoop.h"
#include "Components/TextBlock.h"
#include "MGAndroidBillingResult.h"

void UFunctionsLibrary::SetHighScoreText(UTextBlock* TextBlock, int32 NewHighScore)
{
	if (TextBlock)
	{
		TextBlock->SetText(FText::FromString(FString::Printf(TEXT("Best: %d"), NewHighScore)));
	}
}

bool UFunctionsLibrary::BillingResponseOK(UMGAndroidBillingResult* BillingResult, const TCHAR* Context)
{
	if (!BillingResult)
	{
		UE_LOG(LogFlappyHoop, Warning, TEXT("[Billing] %s: null result"), Context);
		return false;
	}

	const EMGAndroidBillingResponseCode Code = BillingResult->GetResponseCode();
	const bool bOk = Code == EMGAndroidBillingResponseCode::Ok;

	UE_LOG(LogFlappyHoop, Log, TEXT("[Billing] %s: code=%d ok=%s msg=%s"),
		Context, static_cast<int32>(Code), bOk ? TEXT("true") : TEXT("false"), *BillingResult->GetDebugMessage());

	return bOk;
}
