// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/Common/PopupWidget.h"
#include "SecondChanceWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;
class UCircularThrobber;
class URichTextBlock;
class UTotalGemsWidget;
class UMaterialInstanceDynamic;

/**
 * "Continue?" panel shown when the time ran out and the ball hit the ground.
 * Watch an ad or spend gems to continue; skipping (or letting the countdown run out) ends the run.
 * The countdown is a round progress ring (material with "Percent" and "Color" parameters) that drains
 * from green to red, with the seconds left written inside it.
 * Pops in/out like every popup (UPopupWidget); the countdown starts once the pop-in has finished.
 */
UCLASS()
class FLAPPYHOOP_API USecondChanceWidget : public UPopupWidget
{
	GENERATED_BODY()

public:

	/** Called by the game mode when the rewarded ad failed to load or show. Re-opens the panel if needed. */
	void HandleRewardedAdFailed(const FString& Message);

protected:

	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

	/** Refreshes the buttons and shows a full ring. */
	virtual void OnPopupOpening() override;

	/** The skip countdown starts only once the pop-in has finished. */
	virtual void OnPopupOpened() override;

	virtual void OnPopupClosing() override;

private:

	void OnRewardedAdShown();

	// --- Countdown ---
	void StartSkipCountdown();
	void StopSkipCountdown();
	void TickSkipCountdown();

	/** Ring fill, ring color and countdown text for the given remaining time. */
	void UpdateCountdownVisuals(float SecondsRemaining);

	/** Green -> yellow -> red as Fraction (time left / total) goes from 1 to 0. */
	FLinearColor GetCountdownColor(float Fraction) const;

	void SetBusy(bool bBusy);
	void RefreshOptions();
	void ChangeMainText(const FString& NewText, bool bErrorText);

	void OnRewardedAdFailed(const FString& Message);

	UFUNCTION()
	void WatchAdButtonClicked();

	UFUNCTION()
	void SkipAdButtonClicked();

	UFUNCTION()
	void GemButtonClicked();

	/** Seconds before the run ends on its own. */
	UPROPERTY(EditDefaultsOnly, Category = "Second Chance", meta = (ClampMin = "1"))
	int32 MaxSkipTime = 12;

	/** Text on the gem button. {Gems} is replaced with the number of gems needed. */
	UPROPERTY(EditDefaultsOnly, Category = "Second Chance")
	FText GemButtonFormat = NSLOCTEXT("SecondChance", "GemButtonFormat", "Use {Gems}");

	/**
	 * Main text (rich text). {Gems} = gem cost. <img id="Gem"/> needs an image decorator on MainText with a
	 * "Gem" row. Colors come from MainText's Text Style Set: <GemText> (gem part), <AdText> (pink "Watch an Ad"),
	 * untagged text uses the "Default" row (white).
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Second Chance|Text")
	FText BothOptionsText = NSLOCTEXT("SecondChance", "BothOptions", "<GemText>Use {Gems}</> <img id=\"Gem\"/>\nor <AdText>Watch an Ad</> for a second chance");

	UPROPERTY(EditDefaultsOnly, Category = "Second Chance|Text")
	FText AdOnlyText = NSLOCTEXT("SecondChance", "AdOnly", "<AdText>Watch an Ad</> for a second chance");

	UPROPERTY(EditDefaultsOnly, Category = "Second Chance|Text")
	FText GemsOnlyText = NSLOCTEXT("SecondChance", "GemsOnly", "<GemText>Use {Gems}</> <img id=\"Gem\"/> for a second chance");

	/** Ring and countdown text color with the full time left. */
	UPROPERTY(EditDefaultsOnly, Category = "Countdown")
	FLinearColor FullTimeColor = FLinearColor(0.1f, 0.85f, 0.2f);

	/** Color at half time. */
	UPROPERTY(EditDefaultsOnly, Category = "Countdown")
	FLinearColor HalfTimeColor = FLinearColor(1.0f, 0.8f, 0.0f);

	/** Color when time is almost up. */
	UPROPERTY(EditDefaultsOnly, Category = "Countdown")
	FLinearColor LowTimeColor = FLinearColor(0.95f, 0.1f, 0.1f);

	/** Scalar parameter of the ring material, 1 = full ring. */
	UPROPERTY(EditDefaultsOnly, Category = "Countdown")
	FName RingPercentParameter = TEXT("Percent");

	/** Vector parameter of the ring material. */
	UPROPERTY(EditDefaultsOnly, Category = "Countdown")
	FName RingColorParameter = TEXT("Color");

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> WatchAdButton;

	/** Icon inside WatchAdButton; swapped with LoadingAdThrobber (same overlay) while an ad loads. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> WatchAdImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> GemButton;

	/** Optional: text inside GemButton showing the gem cost. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> GemButtonText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SkipAdButton;

	/** Optional: the player's gem balance. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTotalGemsWidget> TotalGemsWidget;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SkipCountdownText;

	/** Round progress ring; its brush must use the round progress bar material. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> CountdownRing;

	/** Inside WatchAdButton, on top of WatchAdImage. Hidden unless an ad is loading. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCircularThrobber> LoadingAdThrobber;

	/** Optional: "Don't give up yet!" message and ad errors. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<URichTextBlock> MainText;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> RingMaterial;

	FTimerHandle SkipTimerHandle;

	float SecondsLeft = 0.0f;
};
