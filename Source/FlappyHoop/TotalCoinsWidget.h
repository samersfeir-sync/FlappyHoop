// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TotalCoinsWidget.generated.h"

class UTextBlock;

UCLASS()
class FLAPPYHOOP_API UTotalCoinsWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	
	void UpdateCoinsText(int NewTotalCoins);

private:

	UPROPERTY(meta = (BindWidget))
	UTextBlock* CoinsText;
};
