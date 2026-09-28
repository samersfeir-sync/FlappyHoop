// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/Common/PopupWidget.h"

namespace
{
	/** 0 -> 1 with an overshoot past 1 before settling ("ease out back"). */
	float EaseOutBack(float T, float Overshoot)
	{
		const float U = T - 1.0f;
		return 1.0f + (Overshoot + 1.0f) * U * U * U + Overshoot * U * U;
	}

	float EaseInQuad(float T)
	{
		return T * T;
	}
}

void UPopupWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (IsDesignTime())
	{
		return;
	}

	// Closed = scale 0 and not tappable. (Never Hidden/Collapsed: it's only ever scaled.)
	SetVisibility(ESlateVisibility::HitTestInvisible);
	ApplyScale(0.0f);
}

void UPopupWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (IsDesignTime())
	{
		ApplyScale(bShowInDesigner ? 1.0f : 0.0f);
	}
}

void UPopupWidget::NativeDestruct()
{
	StopTween();
	Super::NativeDestruct();
}

void UPopupWidget::BeginDestroy()
{
	StopTween(); // never leave a ticker pointing at a dead widget
	Super::BeginDestroy();
}

void UPopupWidget::Open()
{
	if (bOpen)
	{
		return;
	}

	bOpen = true;
	SetVisibility(ESlateVisibility::Visible); // tappable again (also undoes a Blueprint hiding it)
	OnPopupOpening();
	StartTween(1.0f, OpenDuration);
}

void UPopupWidget::Close()
{
	if (!bOpen)
	{
		return;
	}

	bOpen = false;
	SetVisibility(ESlateVisibility::HitTestInvisible); // no taps while it shrinks
	OnPopupClosing();
	StartTween(0.0f, CloseDuration);
}

void UPopupWidget::CloseImmediately()
{
	const bool bWasOpen = bOpen;
	bOpen = false;

	if (bWasOpen)
	{
		OnPopupClosing();
	}

	StopTween();
	SetVisibility(ESlateVisibility::HitTestInvisible);
	ApplyScale(0.0f);

	if (bWasOpen)
	{
		OnPopupClosed();
	}
}

void UPopupWidget::StartTween(float TargetScale, float Duration)
{
	StopTween();

	TweenFrom = GetRenderTransform().Scale.X; // start from where it is, so a quick open/close doesn't jump
	TweenTo = TargetScale;
	TweenDuration = Duration;
	TweenElapsed = 0.0f;

	if (Duration <= 0.0f)
	{
		FinishTween();
		return;
	}

	// Core ticker = real time, keeps running while the game is paused (world timers don't).
	TweenHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UPopupWidget::TickTween));
}

void UPopupWidget::StopTween()
{
	if (TweenHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TweenHandle);
		TweenHandle.Reset();
	}
}

bool UPopupWidget::TickTween(float DeltaTime)
{
	TweenElapsed += DeltaTime;
	const float T = FMath::Clamp(TweenElapsed / TweenDuration, 0.0f, 1.0f);

	const bool bOpening = TweenTo > TweenFrom;
	const float Alpha = bOpening ? EaseOutBack(T, OpenOvershoot) : EaseInQuad(T);
	ApplyScale(FMath::Lerp(TweenFrom, TweenTo, Alpha));

	if (T >= 1.0f)
	{
		TweenHandle.Reset(); // returning false removes the ticker
		FinishTween();
		return false;
	}
	return true;
}

void UPopupWidget::FinishTween()
{
	ApplyScale(TweenTo);

	if (TweenTo > 0.0f)
	{
		if (bOpen)
		{
			OnPopupOpened();
		}
	}
	else if (!bOpen)
	{
		OnPopupClosed();
	}
}

void UPopupWidget::ApplyScale(float Scale)
{
	// Negative scale would mirror the widget; the overshoot only goes above 1, but clamp anyway.
	const float Clamped = FMath::Max(0.0f, Scale);
	SetRenderScale(FVector2D(Clamped, Clamped));
}
