#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "GemShopInfo.generated.h"

USTRUCT(BlueprintType)
struct FGemShopInfo
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	FString Price;

	UPROPERTY(EditDefaultsOnly)
	int32 Amount;

	UPROPERTY(EditDefaultsOnly)
	UTexture2D* Image;

	UPROPERTY(EditDefaultsOnly)
	FString ProductId;

	FGemShopInfo()
		: Price(""), Amount(0), Image(nullptr), ProductId("")
	{
	}
};
