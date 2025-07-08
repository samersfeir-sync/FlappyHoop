// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TotalGemsWidget.generated.h"

class UTextBlock;

UCLASS()
class FLAPPYHOOP_API UTotalGemsWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	void UpdateGemsText(int NewTotalGems);

private:

	UPROPERTY(meta = (BindWidget))
	UTextBlock* GemsText;
};