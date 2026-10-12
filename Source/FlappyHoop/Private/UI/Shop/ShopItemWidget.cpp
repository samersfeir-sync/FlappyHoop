// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Shop/ShopItemWidget.h"
#include "Systems/BallCatalogSubsystem.h"
#include "Systems/EconomySubsystem.h"
#include "UI/Shop/ShopWidget.h"
#include "UI/Shop/InsufficientCoinsWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Kismet/GameplayStatics.h"

void UShopItemWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BuyButton->OnClicked.AddUniqueDynamic(this, &UShopItemWidget::OnBuyButtonClicked);
}

void UShopItemWidget::SetEntry(const FBallCatalogEntry& NewEntry, bool bNewOwned)
{
	Entry = NewEntry;
	bOwned = bNewOwned;

	ItemImage->SetBrushFromTexture(Entry.Icon, false);
	CoinImage->SetVisibility(bOwned ? ESlateVisibility::Hidden : ESlateVisibility::HitTestInvisible);
	SetItemPriceText(bOwned ? OwnedText : FText::FromString(FString::FromInt(Entry.Price)), bOwned);
}

void UShopItemWidget::SetItemPriceText(const FText& PriceText, bool bOwnedStyle)
{
	ItemPriceText->SetText(PriceText);

	// UTextBlock::Font direct access is deprecated since 5.1; use the getter/setter.
	FSlateFontInfo FontInfo = ItemPriceText->GetFont();
	if (DefaultPriceFontSize <= 0)
	{
		DefaultPriceFontSize = FontInfo.Size; // remember the designer size the first time
	}

	ItemPriceText->SetRenderTranslation(bOwnedStyle ? OwnedTextOffset : PriceTextOffset);

	const int32 WantedSize = (bOwnedStyle && OwnedFontSize > 0) ? OwnedFontSize : DefaultPriceFontSize;
	if (FontInfo.Size != WantedSize)
	{
		FontInfo.Size = WantedSize;
		ItemPriceText->SetFont(FontInfo);
	}
}

void UShopItemWidget::OnBuyButtonClicked()
{
	UBallCatalogSubsystem* Catalog = UBallCatalogSubsystem::Get(this);
	const UEconomySubsystem* Economy = UEconomySubsystem::Get(this);
	if (!Catalog || !Economy)
	{
		return;
	}

	if (bOwned)
	{
		Catalog->Select(Entry.BallType); // the shop refreshes through OnBallsChanged
		return;
	}

	const int32 Coins = Economy->GetCoins();
	if (Coins >= Entry.Price)
	{
		PurchaseItem(0);
		return;
	}

	if (!ParentWidget || !ParentWidget->InsufficientCoinsWidget)
	{
		return;
	}

	const int32 MissingCoins = Entry.Price - Coins;
	const int32 GemsNeeded = FMath::Max(1, FMath::RoundToInt(MissingCoins * GemsNeededMultiplier));

	UInsufficientCoinsWidget* InsufficientCoinsWidget = ParentWidget->InsufficientCoinsWidget;
	InsufficientCoinsWidget->SetGemsNeeded(GemsNeeded);
	InsufficientCoinsWidget->SetShopItemWidget(this);
	InsufficientCoinsWidget->SetInsufficientCoinsText(GemsNeeded);
	InsufficientCoinsWidget->SetGemsAmountText(GemsNeeded);
	InsufficientCoinsWidget->Open();
}

void UShopItemWidget::PurchaseItem(int32 GemsUsed)
{
	UBallCatalogSubsystem* Catalog = UBallCatalogSubsystem::Get(this);
	UEconomySubsystem* Economy = UEconomySubsystem::Get(this);
	if (!Catalog || !Economy || !Catalog->Find(Entry.BallType) || Catalog->IsOwned(Entry.BallType))
	{
		return;
	}

	// Gems cover whatever coins are missing, so spend all the coins you have up to the price.
	const int32 CoinsUsed = FMath::Min(Economy->GetCoins(), Entry.Price);
	if (!Economy->TrySpend(CoinsUsed, GemsUsed))
	{
		return;
	}

	UGameplayStatics::PlaySound2D(this, PurchaseSound);

	// Saves ownership + selection and broadcasts OnBallsChanged, which rebuilds the shop (this widget is replaced).
	Catalog->Unlock(Entry.BallType);
}

void UShopItemWidget::UpdateBorderColor(EBallType SelectedBallType)
{
	if (!MainBorder)
	{
		return;
	}

	FSlateBrush Brush = MainBorder->Background;
	Brush.OutlineSettings.Color = (Entry.BallType == SelectedBallType) ? SelectedOutlineColor : DefaultOutlineColor;
	MainBorder->SetBrush(Brush);
}
