// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Containers/Ticker.h"
#include "PopupWidget.generated.h"

/**
 * Base class for every popup (settings, shop, game over, second chance, pause, not-enough-coins).
 *
 * A popup is never hidden: it is shown/hidden only by scaling it from 0 to 1 (pop in, with a small overshoot)
 * and back to 0. While closed (and while shrinking) it ignores taps, so nothing behind it gets blocked.
 * The animation runs on real time, so it also plays while the game is paused.
 *
 * Children react to the hooks below instead of overriding Open/Close.
 */
UCLASS(Abstract)
class FLAPPYHOOP_API UPopupWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/** Pops the popup in. Does nothing if it's already open (or opening). */
	UFUNCTION(BlueprintCallable, Category = "Popup")
	void Open();

	/** Pops the popup out. Does nothing if it's already closed (or closing). */
	UFUNCTION(BlueprintCallable, Category = "Popup")
	void Close();

	/** Closes without animation (e.g. when the whole screen changes). */
	UFUNCTION(BlueprintCallable, Category = "Popup")
	void CloseImmediately();

	/** True from Open() until Close(). Buttons should ignore taps while this is false (they may still be shrinking). */
	UFUNCTION(BlueprintPure, Category = "Popup")
	bool IsOpen() const { return bOpen; }

protected:

	virtual void NativeOnInitialized() override;
	virtual void NativePreConstruct() override;
	virtual void NativeDestruct() override;
	virtual void BeginDestroy() override;

	/** Right before the pop-in starts: fill in texts, refresh buttons. */
	virtual void OnPopupOpening() {}

	/** Pop-in finished (fully visible). */
	virtual void OnPopupOpened() {}

	/** Right when Close() is called: stop timers etc. */
	virtual void OnPopupClosing() {}

	/** Pop-out finished (scale 0). */
	virtual void OnPopupClosed() {}

	/** Seconds to pop in. */
	UPROPERTY(EditDefaultsOnly, Category = "Popup", meta = (ClampMin = "0"))
	float OpenDuration = 0.25f;

	/** Seconds to pop out. */
	UPROPERTY(EditDefaultsOnly, Category = "Popup", meta = (ClampMin = "0"))
	float CloseDuration = 0.15f;

	/** How far the pop-in overshoots before settling at 1 (0 = no overshoot, 1.7 = classic "back" ease). */
	UPROPERTY(EditDefaultsOnly, Category = "Popup", meta = (ClampMin = "0"))
	float OpenOvershoot = 1.7f;

	/** Keep the popup at full size in the widget designer so it can be edited. */
	UPROPERTY(EditAnywhere, Category = "Popup")
	bool bShowInDesigner = true;

private:

	void StartTween(float TargetScale, float Duration);
	void StopTween();
	bool TickTween(float DeltaTime);
	void ApplyScale(float Scale);
	void FinishTween();

	bool bOpen = false;

	float TweenFrom = 0.0f;
	float TweenTo = 0.0f;
	float TweenDuration = 0.0f;
	float TweenElapsed = 0.0f;

	FTSTicker::FDelegateHandle TweenHandle;
};
