#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "GemShopInfo.generated.h"

USTRUCT(BlueprintType)
struct FGemShopInfo
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Shop")
	FString Price;

	UPROPERTY(EditDefaultsOnly, Category = "Shop")
	int32 Amount;

	UPROPERTY(EditDefaultsOnly, Category = "Shop")
	TObjectPtr<UTexture2D> Image;

	UPROPERTY(EditDefaultsOnly, Category = "Shop")
	FString ProductId;

	FGemShopInfo()
		: Price(""), Amount(0), Image(nullptr), ProductId("")
	{
	}
};
