// Fill out your copyright notice in the Description page of Project Settings.


#include "ShopItemWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "GameInstanceInterface.h"
#include "UserProgression.h"
#include "GameModeInterface.h"
#include "ShopWidget.h"
#include "Kismet/GameplayStatics.h"

void UShopItemWidget::SetItemImage(UTexture2D* ItemTexture)
{
	ItemImage->SetBrushFromTexture(ItemTexture, false);
}

void UShopItemWidget::SetItemPriceText(const FString& PriceText, bool AdjustFont)
{
	ItemPriceText->SetText(FText::FromString(PriceText));

	if (AdjustFont)
	{
		ItemPriceText->SetRenderTranslation(FVector2D(-54, 0));
		ItemPriceText->Font.Size = 35;
	}
}

void UShopItemWidget::SetCoinImageVisibility(bool Visible)
{
	ESlateVisibility SizeBoxVisibility = Visible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden;
	CoinImage->SetVisibility(SizeBoxVisibility);
}

void UShopItemWidget::OnBuyButtonClicked()
{
	if (GameInstanceInterface)
	{
		FUserProgression UserProgession = GameInstanceInterface->GetUserProgression();

		if (!BallShopStruct.IsPurchased)
		{
			if (UserProgession.TotalCoins >= BallShopStruct.Price)
			{
				int32 IndexFound = UserProgession.BallsOwned.Find(BallShopStruct);

				if (IndexFound != INDEX_NONE)
				{
					UserProgession.BallsOwned[IndexFound].IsPurchased = true;
					UserProgession.TotalCoins -= BallShopStruct.Price;
					UserProgession.BallType = BallShopStruct.BallType;
					BallShopStruct.IsPurchased = true;
					GameModeInterface->SetBallType(BallShopStruct.BallType);
					GameModeInterface->ApplyBallSettings();
					GameInstanceInterface->SaveUserProgression(UserProgession);
					OnBallPurchased.Broadcast(UserProgession.BallsOwned);
					UGameplayStatics::PlaySound2D(this, PurchaseSound);
				}
			}
		}
		else
		{
			UserProgession.BallType = BallShopStruct.BallType;
			GameInstanceInterface->SaveUserProgression(UserProgession);
			GameModeInterface->SetBallType(BallShopStruct.BallType);
			GameModeInterface->ApplyBallSettings();
			UpdateBorderColor();
		}
	}
}

void UShopItemWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BuyButton->OnClicked.AddDynamic(this, &UShopItemWidget::OnBuyButtonClicked);
	UpdateBorderColor();
}

void UShopItemWidget::UpdateBorderColor()
{
	const FLinearColor DefaultOutlineColor(0.38f, 0.15f, 0.0f, 1.0f);

	EBallType SelectedBallType = GameModeInterface->GetBallType();

	for (UShopItemWidget* ShopItem : ParentWidget->ShopItemWidgets)
	{
		FLinearColor DesiredColor = (ShopItem->BallShopStruct.BallType == SelectedBallType) ? SelectedOutlineColor : DefaultOutlineColor;
		FSlateBrush ShopItemBrush = ShopItem->MainBorder->Background;
		ShopItemBrush.OutlineSettings.Color = DesiredColor;
		ShopItem->MainBorder->SetBrush(ShopItemBrush);
	}
}