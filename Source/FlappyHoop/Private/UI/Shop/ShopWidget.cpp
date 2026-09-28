// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Shop/ShopWidget.h"
#include "Core/FlappyHoop.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/UniformGridPanel.h"
#include "Systems/BallCatalogSubsystem.h"
#include "Systems/StoreSubsystem.h"
#include "Systems/EconomySubsystem.h"
#include "UI/Shop/ShopItemWidget.h"
#include "UI/Shop/GemShopWidget.h"
#include "UI/Shop/InsufficientCoinsWidget.h"
#include "UI/HUD/TotalCoinsWidget.h"
#include "UI/HUD/TotalGemsWidget.h"

void UShopWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (IsDesignTime())
	{
		FillItemContainer();
	}

	FillGemsContainer();
}

void UShopWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BackButton->OnClicked.AddUniqueDynamic(this, &UShopWidget::HideShopWidget);
	BallsButton->OnClicked.AddUniqueDynamic(this, &UShopWidget::BallButtonClicked);
	GemsButton->OnClicked.AddUniqueDynamic(this, &UShopWidget::GemButtonClicked);

	InsufficientCoinsWidget->SetShopWidget(this);

	FillItemContainer();
	if (UBallCatalogSubsystem* Catalog = UBallCatalogSubsystem::Get(this))
	{
		Catalog->OnBallsChanged.AddUObject(this, &UShopWidget::FillItemContainer);
	}

	// Hand the gem table to the store, which owns the purchase -> consume -> credit flow.
	if (UStoreSubsystem* Store = UStoreSubsystem::Get(this))
	{
		TMap<FString, int32> GemAmounts;
		for (const FGemShopInfo& Info : GemShopInfo)
		{
			if (!Info.ProductId.IsEmpty())
			{
				GemAmounts.Add(Info.ProductId, Info.Amount);
			}
		}
		Store->RegisterGemProducts(GemAmounts);
		Store->OnPricesUpdated.AddUObject(this, &UShopWidget::OnStorePricesUpdated);
	}
	OnStorePricesUpdated(); // prices may already be known

	if (UEconomySubsystem* Economy = UEconomySubsystem::Get(this))
	{
		Economy->OnBalanceChanged.AddUObject(this, &UShopWidget::OnBalanceChanged);
	}
}

void UShopWidget::NativeDestruct()
{
	if (UEconomySubsystem* Economy = UEconomySubsystem::Get(this))
	{
		Economy->OnBalanceChanged.RemoveAll(this);
	}

	if (UBallCatalogSubsystem* Catalog = UBallCatalogSubsystem::Get(this))
	{
		Catalog->OnBallsChanged.RemoveAll(this);
	}

	if (UStoreSubsystem* Store = UStoreSubsystem::Get(this))
	{
		Store->OnPricesUpdated.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UShopWidget::FillItemContainer()
{
	if (!ItemsGridPanel || !ShopItemWidgetClass)
	{
		return;
	}

	ItemsGridPanel->ClearChildren();
	ShopItemWidgets.Reset();

	if (IsDesignTime())
	{
		// No game instance in the widget editor: preview the assigned catalog, or the old preview list.
		int32 Index = 0;
		if (PreviewCatalog)
		{
			for (const FBallCatalogEntry& Entry : PreviewCatalog->Balls)
			{
				AddBallTile(Entry, Entry.bOwnedByDefault, Index++);
			}
		}
		else
		{
			for (const FBallsShopStruct& Legacy : BallShopItems)
			{
				FBallCatalogEntry Entry;
				Entry.BallType = Legacy.BallType;
				Entry.Icon = Legacy.BallTexture;
				Entry.Price = Legacy.Price;
				AddBallTile(Entry, Legacy.IsPurchased, Index++);
			}
		}
		return;
	}

	const UBallCatalogSubsystem* Catalog = UBallCatalogSubsystem::Get(this);
	if (!Catalog)
	{
		return;
	}

	const TArray<FBallCatalogEntry>& Balls = Catalog->GetBalls();
	for (int32 i = 0; i < Balls.Num(); ++i)
	{
		AddBallTile(Balls[i], Catalog->IsOwned(Balls[i].BallType), i);
	}

	const EBallType Selected = Catalog->GetSelected();
	for (UShopItemWidget* Item : ShopItemWidgets)
	{
		Item->UpdateBorderColor(Selected);
	}

	RefreshTotals();
}

void UShopWidget::AddBallTile(const FBallCatalogEntry& Entry, bool bOwned, int32 Index)
{
	UShopItemWidget* NewItem = CreateWidget<UShopItemWidget>(this, ShopItemWidgetClass);
	if (!NewItem)
	{
		return;
	}

	constexpr int32 ItemsPerRow = 3;

	ShopItemWidgets.Add(NewItem);
	NewItem->SetParentWidgetReference(this);
	NewItem->SetEntry(Entry, bOwned);
	ItemsGridPanel->AddChildToUniformGrid(NewItem, Index / ItemsPerRow, Index % ItemsPerRow);
}

void UShopWidget::FillGemsContainer()
{
	if (!GemsGridPanel || !GemShopWidgetClass || GemShopInfo.IsEmpty())
	{
		return;
	}

	// PreConstruct can run more than once; don't duplicate the gem packs.
	GemsGridPanel->ClearChildren();
	GemShopWidgets.Reset();

	constexpr int32 GemsPerRow = 2;

	for (int32 i = 0; i < GemShopInfo.Num(); ++i)
	{
		UGemShopWidget* NewGemItem = CreateWidget<UGemShopWidget>(this, GemShopWidgetClass);
		if (!NewGemItem)
		{
			continue;
		}

		const FGemShopInfo& Info = GemShopInfo[i];
		NewGemItem->SetParentWidget(this);
		NewGemItem->SetProductId(Info.ProductId); // before the price, so the localized one can be looked up
		NewGemItem->SetGemPrice(Info.Price);
		NewGemItem->SetGemAmount(Info.Amount);
		NewGemItem->SetGemImage(Info.Image);
		GemShopWidgets.Add(NewGemItem);

		GemsGridPanel->AddChildToUniformGrid(NewGemItem, i / GemsPerRow, i % GemsPerRow);
	}
}

void UShopWidget::OnStorePricesUpdated()
{
	for (UGemShopWidget* GemItem : GemShopWidgets)
	{
		if (GemItem)
		{
			GemItem->RefreshPrice();
		}
	}
}

void UShopWidget::RefreshTotals()
{
	if (const UEconomySubsystem* Economy = UEconomySubsystem::Get(this))
	{
		OnBalanceChanged(Economy->GetCoins(), Economy->GetGems());
	}
}

void UShopWidget::OnBalanceChanged(int32 Coins, int32 Gems)
{
	TotalCoinsWidget->UpdateCoinsText(Coins);
	TotalGemsWidget->UpdateGemsText(Gems);
}

void UShopWidget::HideShopWidget()
{
	Close();
}

void UShopWidget::OnPopupOpening()
{
	RefreshTotals();
}

void UShopWidget::OnPopupClosed()
{
	InsufficientCoinsWidget->CloseImmediately();
	ScrollBox->ScrollToStart();
}

void UShopWidget::BallButtonClicked()
{
	ItemsGridPanel->SetVisibility(ESlateVisibility::Visible);
	GemsGridPanel->SetVisibility(ESlateVisibility::Collapsed);
	BallsButton->SetIsEnabled(false);
	GemsButton->SetIsEnabled(true);
	ScrollBox->ScrollToStart();
}

void UShopWidget::GemButtonClicked()
{
	ItemsGridPanel->SetVisibility(ESlateVisibility::Collapsed);
	GemsGridPanel->SetVisibility(ESlateVisibility::Visible);
	BallsButton->SetIsEnabled(true);
	GemsButton->SetIsEnabled(false);
	ScrollBox->ScrollToStart();
}
