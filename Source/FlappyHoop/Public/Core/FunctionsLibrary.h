// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FunctionsLibrary.generated.h"

class UTextBlock;
class UMGAndroidBillingResult;

UCLASS()
class FLAPPYHOOP_API UFunctionsLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	static void SetHighScoreText(UTextBlock* TextBlock, int32 NewHighScore);

	/** True when the billing result is non-null and its response code is Ok. Logs the result either way. */
	static bool BillingResponseOK(UMGAndroidBillingResult* BillingResult, const TCHAR* Context = TEXT(""));
};
