// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/Popups/SecondChanceWidget.h"
#include "Core/FlappyHoop.h"
#include "Framework/BaseGameMode.h"
#include "Systems/EconomySubsystem.h"
#include "UI/HUD/TotalGemsWidget.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/CircularThrobber.h"
#include "Components/RichTextBlock.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"

namespace
{
	/** How often the ring is redrawn while counting down. */
	constexpr float CountdownStep = 0.05f;
}

void USecondChanceWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	SkipAdButton->OnClicked.AddUniqueDynamic(this, &USecondChanceWidget::SkipAdButtonClicked);
	WatchAdButton->OnClicked.AddUniqueDynamic(this, &USecondChanceWidget::WatchAdButtonClicked);
	GemButton->OnClicked.AddUniqueDynamic(this, &USecondChanceWidget::GemButtonClicked);

	// Own copy of the ring material so its parameters can change at runtime.
	RingMaterial = CountdownRing->GetDynamicMaterial();
	if (!RingMaterial)
	{
		UE_LOG(LogFlappyHoop, Warning, TEXT("SecondChance: CountdownRing brush has no material, the ring will not animate"));
	}

	if (ABaseGameMode* GameMode = ABaseGameMode::Get(this))
	{
		GameMode->OnRewardedAdShown.AddUObject(this, &USecondChanceWidget::OnRewardedAdShown);
		GameMode->OnRewardedAdFailed.AddUObject(this, &USecondChanceWidget::HandleRewardedAdFailed);
	}
}

void USecondChanceWidget::NativeDestruct()
{
	if (ABaseGameMode* GameMode = ABaseGameMode::Get(this))
	{
		GameMode->OnRewardedAdShown.RemoveAll(this);
		GameMode->OnRewardedAdFailed.RemoveAll(this);
	}

	StopSkipCountdown();
	Super::NativeDestruct();
}

// ---------------------------------------------------------------------------
// Open / close
// ---------------------------------------------------------------------------

void USecondChanceWidget::OnPopupOpening()
{
	StopSkipCountdown();
	SetBusy(false);
	RefreshOptions();

	// Full green ring while the panel pops in; the countdown starts in OnPopupOpened.
	UpdateCountdownVisuals(static_cast<float>(MaxSkipTime));
}

void USecondChanceWidget::OnPopupOpened()
{
	StartSkipCountdown();
}

void USecondChanceWidget::OnPopupClosing()
{
	StopSkipCountdown();
}

void USecondChanceWidget::OnRewardedAdShown()
{
	Close();
}

void USecondChanceWidget::RefreshOptions()
{
	const ABaseGameMode* GameMode = ABaseGameMode::Get(this);
	if (!GameMode)
	{
		return;
	}

	const UEconomySubsystem* Economy = UEconomySubsystem::Get(this);
	const bool bCanWatchAd = GameMode->GetCanWatchAd();
	const int32 GemsNeeded = GameMode->GetGemsNeededForSecondChance();
	const int32 TotalGems = Economy ? Economy->GetGems() : 0;
	const bool bCanUseGems = TotalGems >= GemsNeeded;

	WatchAdButton->SetIsEnabled(bCanWatchAd);
	GemButton->SetIsEnabled(bCanUseGems);
	if (GemButtonText)
	{
		GemButtonText->SetText(FText::FormatNamed(GemButtonFormat, TEXT("Gems"), GemsNeeded));
	}
	if (TotalGemsWidget)
	{
		TotalGemsWidget->UpdateGemsText(TotalGems);
	}

	if (!MainText)
	{
		return;
	}

	// Families policy: say plainly which option is an ad. {Gems} = gem cost.
	const FText& Format = (bCanWatchAd && bCanUseGems) ? BothOptionsText
		: bCanWatchAd ? AdOnlyText
		: GemsOnlyText;

	ChangeMainText(FText::FormatNamed(Format, TEXT("Gems"), GemsNeeded).ToString(), false);
}

/**
 * While an ad is loading, every button is locked and the throbber replaces the Watch Ad icon.
 * The countdown stays visible (paused).
 */
void USecondChanceWidget::SetBusy(bool bBusy)
{
	LoadingAdThrobber->SetVisibility(bBusy ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	WatchAdImage->SetVisibility(bBusy ? ESlateVisibility::Hidden : ESlateVisibility::HitTestInvisible);
	SkipAdButton->SetIsEnabled(!bBusy);

	if (bBusy)
	{
		WatchAdButton->SetIsEnabled(false);
		GemButton->SetIsEnabled(false);
	}
}

void USecondChanceWidget::ChangeMainText(const FString& NewText, bool bErrorText)
{
	if (!MainText)
	{
		return;
	}

	const FString Text = bErrorText ? FString::Printf(TEXT("<RedText>%s</>"), *NewText) : NewText;
	MainText->SetText(FText::FromString(Text));
}

// ---------------------------------------------------------------------------
// Skip countdown
// ---------------------------------------------------------------------------

void USecondChanceWidget::StartSkipCountdown()
{
	SecondsLeft = static_cast<float>(MaxSkipTime);
	UpdateCountdownVisuals(SecondsLeft);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(SkipTimerHandle, this, &USecondChanceWidget::TickSkipCountdown, CountdownStep, true);
	}
}

void USecondChanceWidget::StopSkipCountdown()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SkipTimerHandle);
	}
}

void USecondChanceWidget::TickSkipCountdown()
{
	SecondsLeft = FMath::Max(0.0f, SecondsLeft - CountdownStep);
	UpdateCountdownVisuals(SecondsLeft);

	if (SecondsLeft <= 0.0f)
	{
		SkipAdButtonClicked();
	}
}

void USecondChanceWidget::UpdateCountdownVisuals(float SecondsRemaining)
{
	const float Fraction = MaxSkipTime > 0 ? FMath::Clamp(SecondsRemaining / MaxSkipTime, 0.0f, 1.0f) : 0.0f;
	const FLinearColor Color = GetCountdownColor(Fraction);

	if (RingMaterial)
	{
		RingMaterial->SetScalarParameterValue(RingPercentParameter, Fraction);
		RingMaterial->SetVectorParameterValue(RingColorParameter, Color);
	}

	// Whole seconds, rounded up: shows 12 at the start and 1 during the last second.
	SkipCountdownText->SetText(FText::AsNumber(FMath::CeilToInt(SecondsRemaining)));
	SkipCountdownText->SetColorAndOpacity(FSlateColor(Color));
}

FLinearColor USecondChanceWidget::GetCountdownColor(float Fraction) const
{
	return Fraction >= 0.5f
		? FLinearColor::LerpUsingHSV(HalfTimeColor, FullTimeColor, (Fraction - 0.5f) * 2.0f)
		: FLinearColor::LerpUsingHSV(LowTimeColor, HalfTimeColor, Fraction * 2.0f);
}

// ---------------------------------------------------------------------------
// Buttons
// ---------------------------------------------------------------------------

void USecondChanceWidget::WatchAdButtonClicked()
{
	// Buttons stay on screen during the close animation; ignore taps then.
	if (!IsOpen())
	{
		return;
	}

	ABaseGameMode* GameMode = ABaseGameMode::Get(this);
	if (!GameMode)
	{
		return;
	}

	StopSkipCountdown();
	SetBusy(true);

	// The game mode broadcasts OnRewardedAdShown (-> Close) or OnRewardedAdFailed (-> HandleRewardedAdFailed).
	GameMode->LoadRewardedAd();
}

void USecondChanceWidget::HandleRewardedAdFailed(const FString& Message)
{
	if (!IsOpen())
	{
		// The ad was loaded (panel closed) but then failed to show: bring the panel back.
		Open();
	}

	OnRewardedAdFailed(Message);
}

void USecondChanceWidget::OnRewardedAdFailed(const FString& Message)
{
	UE_LOG(LogFlappyHoop, Log, TEXT("SecondChance: rewarded ad failed: %s"), *Message);

	SetBusy(false);
	RefreshOptions();
	ChangeMainText(Message.IsEmpty() ? TEXT("The ad couldn't load. Please try again.") : Message, true);
	StartSkipCountdown();
}

void USecondChanceWidget::SkipAdButtonClicked()
{
	if (!IsOpen())
	{
		return;
	}

	Close();

	if (ABaseGameMode* GameMode = ABaseGameMode::Get(this))
	{
		GameMode->EndGame();
	}
}

void USecondChanceWidget::GemButtonClicked()
{
	if (!IsOpen())
	{
		return;
	}

	// Spends the gems and moves the run back to Playing; the game widget closes this panel on the phase change.
	if (ABaseGameMode* GameMode = ABaseGameMode::Get(this))
	{
		if (!GameMode->TryContinueWithGems())
		{
			RefreshOptions(); // balance changed under us; update the buttons
		}
	}
}
