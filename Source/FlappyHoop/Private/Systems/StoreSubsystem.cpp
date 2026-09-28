// Fill out your copyright notice in the Description page of Project Settings.

#include "Systems/StoreSubsystem.h"
#include "Core/FlappyHoop.h"
#include "Core/FunctionsLibrary.h"
#include "Framework/GameInfoInstance.h"
#include "Systems/SaveSubsystem.h"
#include "Systems/EconomySubsystem.h"
#include "Systems/AdsSubsystem.h"
#include "MGAndroidBillingClient.h"
#include "MGAndroidBillingResult.h"
#include "MGAndroidBillingFlowParameters.h"
#include "MGAndroidProductDetails.h"
#include "MGAndroidPurchase.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "TimerManager.h"

UStoreSubsystem* UStoreSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UStoreSubsystem>() : nullptr;
}

void UStoreSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Collection.InitializeDependency<USaveSubsystem>();
	Collection.InitializeDependency<UEconomySubsystem>();
	Collection.InitializeDependency<UAdsSubsystem>();
	Super::Initialize(Collection);

#if PLATFORM_ANDROID
	OnPurchasesUpdatedDelegate.BindDynamic(this, &UStoreSubsystem::OnPurchasesUpdated);
	BillingClient = UMGAndroidBillingLibrary::CreateAndroidBillingClient(OnPurchasesUpdatedDelegate);
	StartConnection();
#endif
}

void UStoreSubsystem::Deinitialize()
{
	if (FTimerManager* Timers = GetTimers())
	{
		Timers->ClearAllTimersForObject(this);
	}

#if PLATFORM_ANDROID
	if (BillingClient && BillingClient->IsNativeObjectValid())
	{
		BillingClient->EndConnection();
	}
#endif

	Super::Deinitialize();
}

FTimerManager* UStoreSubsystem::GetTimers() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? &GameInstance->GetTimerManager() : nullptr;
}

bool UStoreSubsystem::IsReady() const
{
#if PLATFORM_ANDROID
	return BillingClient && BillingClient->IsNativeObjectValid() && BillingClient->IsReady();
#else
	return false;
#endif
}

// ---------------------------------------------------------------------------------------------------------------------
// Connection
// ---------------------------------------------------------------------------------------------------------------------

void UStoreSubsystem::StartConnection()
{
#if PLATFORM_ANDROID
	if (!BillingClient)
	{
		return;
	}

	UE_LOG(LogFlappyHoop, Log, TEXT("[Billing] Connecting (attempt %d)"), ReconnectAttempts + 1);

	OnBillingSetupFinishedDelegate.BindDynamic(this, &UStoreSubsystem::OnBillingSetupFinished);
	OnBillingDisconnectedDelegate.BindDynamic(this, &UStoreSubsystem::OnBillingDisconnected);
	BillingClient->StartConnection(OnBillingSetupFinishedDelegate, OnBillingDisconnectedDelegate);
#endif
}

void UStoreSubsystem::OnBillingSetupFinished(UMGAndroidBillingResult* Result)
{
	if (UFunctionsLibrary::BillingResponseOK(Result, TEXT("BillingSetupFinished")))
	{
		ReconnectAttempts = 0;

		// A dropped connection can swallow ack/consume callbacks; unstick the queues after reconnecting.
		bAcknowledgeInFlight = false;
		bConsumeInFlight = false;

		// A reconnect could have dropped an in-flight product query too.
		bProductQueryInFlight = false;

		QueryOwnedPurchases();
		QueryPrices();
		return;
	}

	OnBillingDisconnected();
}

void UStoreSubsystem::OnBillingDisconnected()
{
	const UGameInfoInstance* GameInstance = Cast<UGameInfoInstance>(GetGameInstance());
	const int32 MaxAttempts = GameInstance ? GameInstance->GetMaxBillingReconnectAttempts() : 6;

	if (IsReady() || ReconnectAttempts >= MaxAttempts)
	{
		return;
	}

	const float Delay = FMath::Min(30.0f, FMath::Pow(2.0f, static_cast<float>(ReconnectAttempts)));
	++ReconnectAttempts;

	if (FTimerManager* Timers = GetTimers())
	{
		Timers->SetTimer(ReconnectTimer, FTimerDelegate::CreateUObject(this, &UStoreSubsystem::StartConnection), Delay, false);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Purchase flow: query product -> launch Play Store sheet. The result arrives in OnPurchasesUpdated.
// ---------------------------------------------------------------------------------------------------------------------

void UStoreSubsystem::Purchase(const FString& ProductId)
{
#if PLATFORM_ANDROID
	if (bPurchaseFlowInProgress || ProductId.IsEmpty() || !IsReady())
	{
		UE_LOG(LogFlappyHoop, Warning, TEXT("[Billing] Can't start purchase for %s (busy or not connected)"), *ProductId);
		return;
	}

	bPurchaseFlowInProgress = true;

	// Already know this product (from the price query): open the Play sheet straight away.
	if (const TObjectPtr<UMGAndroidProductDetails>* Cached = CachedProductDetails.Find(ProductId))
	{
		UMGAndroidProductDetails* Details = Cached->Get();
		if (Details && Details->IsNativeObjectValid())
		{
			LaunchPurchase(Details);
			return;
		}
	}

	if (bProductQueryInFlight)
	{
		PendingPurchaseId = ProductId; // runs when the current query answers
		return;
	}

	bProductQueryInFlight = true;
	const TArray<FString> ProductIds = { ProductId };
	OnProductDetailsDelegate.BindDynamic(this, &UStoreSubsystem::OnProductDetailsQueried);
	BillingClient->QueryProductDetails(EMGSkuType::Purchase, ProductIds, OnProductDetailsDelegate);
#else
	UE_LOG(LogFlappyHoop, Log, TEXT("[Billing] Purchase(%s) ignored: billing is Android-only"), *ProductId);
#endif
}

void UStoreSubsystem::OnProductDetailsQueried(UMGAndroidBillingResult* Result, const TArray<UMGAndroidProductDetails*>& ProductDetails)
{
	bProductQueryInFlight = false;

	if (!UFunctionsLibrary::BillingResponseOK(Result, TEXT("QueryProductDetails")) || ProductDetails.IsEmpty() || !ProductDetails[0] || !IsReady())
	{
		bPurchaseFlowInProgress = false;
		RunPendingProductWork();
		return;
	}

	CacheProductDetails(ProductDetails);
	LaunchPurchase(ProductDetails[0]);
	RunPendingProductWork();
}

void UStoreSubsystem::LaunchPurchase(UMGAndroidProductDetails* Details)
{
#if PLATFORM_ANDROID
	UMGAndroidBillingFlowParameters* Params = UMGAndroidBillingLibrary::CreateAndroidBillingFlowParameters();
	if (!Params || !Details || !IsReady())
	{
		bPurchaseFlowInProgress = false;
		return;
	}

	Params->SetProductDetailsParams(Details);

	OnBillingFlowStartedDelegate.BindDynamic(this, &UStoreSubsystem::OnBillingFlowStarted);
	BillingClient->LaunchBillingFlow(Params, OnBillingFlowStartedDelegate);
#endif
}

void UStoreSubsystem::OnBillingFlowStarted(UMGAndroidBillingResult* Result)
{
	// The purchase sheet has been shown (or failed to). Either way another purchase may be started.
	UFunctionsLibrary::BillingResponseOK(Result, TEXT("LaunchBillingFlow"));
	bPurchaseFlowInProgress = false;
}

// ---------------------------------------------------------------------------------------------------------------------
// Localized prices
// ---------------------------------------------------------------------------------------------------------------------

FString UStoreSubsystem::GetLocalizedPrice(const FString& ProductId) const
{
	const FString* Price = LocalizedPrices.Find(ProductId);
	return Price ? *Price : FString();
}

void UStoreSubsystem::QueryPrices()
{
#if PLATFORM_ANDROID
	if (!IsReady() || bProductQueryInFlight)
	{
		bPriceQueryPending = true; // retried on connect / when the current query answers
		return;
	}

	TArray<FString> ProductIds;
	GemProductAmounts.GetKeys(ProductIds);
	ProductIds.AddUnique(FlappyHoopStore::RemoveAdsProductId);

	bPriceQueryPending = false;
	bProductQueryInFlight = true;

	OnPricesQueriedDelegate.BindDynamic(this, &UStoreSubsystem::OnPricesQueried);
	BillingClient->QueryProductDetails(EMGSkuType::Purchase, ProductIds, OnPricesQueriedDelegate);
#endif
}

void UStoreSubsystem::OnPricesQueried(UMGAndroidBillingResult* Result, const TArray<UMGAndroidProductDetails*>& ProductDetails)
{
	bProductQueryInFlight = false;

	if (UFunctionsLibrary::BillingResponseOK(Result, TEXT("QueryPrices")))
	{
		CacheProductDetails(ProductDetails);
		OnPricesUpdated.Broadcast();
	}

	RunPendingProductWork();
}

void UStoreSubsystem::CacheProductDetails(const TArray<UMGAndroidProductDetails*>& ProductDetails)
{
	for (UMGAndroidProductDetails* Details : ProductDetails)
	{
		if (!Details)
		{
			continue;
		}

		const FString Id = Details->GetProductId();
		if (Id.IsEmpty())
		{
			continue;
		}

		CachedProductDetails.Add(Id, Details);

		const FOneTimePurchaseOfferDetails Offer = Details->GetOneTimePurchaseOfferDetails();
		if (Offer.IsValid && !Offer.FormattedPrice.IsEmpty())
		{
			LocalizedPrices.Add(Id, Offer.FormattedPrice);
			UE_LOG(LogFlappyHoop, Log, TEXT("[Billing] Price %s = %s"), *Id, *Offer.FormattedPrice);
		}
	}
}

void UStoreSubsystem::RunPendingProductWork()
{
	if (bProductQueryInFlight)
	{
		return;
	}

	if (!PendingPurchaseId.IsEmpty())
	{
		const FString Id = PendingPurchaseId;
		PendingPurchaseId.Reset();
		bPurchaseFlowInProgress = false; // Purchase() sets it again
		Purchase(Id);
	}

	// If the purchase started its own query, QueryPrices just stays pending until that answers.
	if (bPriceQueryPending)
	{
		QueryPrices();
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Owned purchases
// ---------------------------------------------------------------------------------------------------------------------

void UStoreSubsystem::QueryOwnedPurchases()
{
	if (!IsReady())
	{
		return;
	}

	// Play Billing 8 removed queryPurchaseHistory. queryPurchases returns everything currently owned,
	// which is exactly what we need to restore "remove ads" and finish unconsumed gem packs.
	OnQueryPurchasesDelegate.BindDynamic(this, &UStoreSubsystem::OnOwnedPurchasesQueried);
	BillingClient->QueryPurchases(EMGSkuType::Purchase, OnQueryPurchasesDelegate);
}

void UStoreSubsystem::OnPurchasesUpdated(UMGAndroidBillingResult* Result, const TArray<UMGAndroidPurchase*>& Purchases)
{
	if (UFunctionsLibrary::BillingResponseOK(Result, TEXT("PurchasesUpdated")))
	{
		ProcessPurchases(Purchases);
	}
}

void UStoreSubsystem::OnOwnedPurchasesQueried(UMGAndroidBillingResult* Result, const TArray<UMGAndroidPurchase*>& Purchases)
{
	if (UFunctionsLibrary::BillingResponseOK(Result, TEXT("QueryPurchases")))
	{
		ProcessPurchases(Purchases);
	}
}

void UStoreSubsystem::ProcessPurchases(const TArray<UMGAndroidPurchase*>& Purchases)
{
	for (UMGAndroidPurchase* Purchase : Purchases)
	{
		ProcessPurchase(Purchase);
	}

	AcknowledgeNext();
	ConsumeNext();
}

void UStoreSubsystem::ProcessPurchase(UMGAndroidPurchase* Purchase)
{
	if (!Purchase || Purchase->GetPurchaseState() != EMGAndroidPurchaseState::Purchased)
	{
		// Pending purchases are delivered again through OnPurchasesUpdated once they complete.
		return;
	}

	const FString Token = Purchase->GetPurchaseToken();
	if (Token.IsEmpty() || HandledPurchaseTokens.Contains(Token))
	{
		return;
	}

	for (const FString& ProductId : Purchase->GetSkus())
	{
		if (ProductId == FlappyHoopStore::RemoveAdsProductId)
		{
			HandledPurchaseTokens.Add(Token);
			GrantRemoveAds();

			if (!Purchase->IsAcknowledged())
			{
				AcknowledgeQueue.AddUnique(Token);
			}
			return;
		}

		if (GemProductAmounts.Contains(ProductId))
		{
			// Consuming a purchase also acknowledges it, so no separate acknowledge needed.
			HandledPurchaseTokens.Add(Token);
			ConsumeQueue.Add({ Token, ProductId });
			return;
		}
	}

	// Unknown product, most likely a gem pack arriving before the shop registered the gem table.
	PurchasesAwaitingGemTable.AddUnique(Purchase);
}

void UStoreSubsystem::RegisterGemProducts(const TMap<FString, int32>& ProductIdToGemAmount)
{
	GemProductAmounts = ProductIdToGemAmount;

	// New product IDs: fetch their localized prices (queued if billing isn't connected yet).
	QueryPrices();

	if (PurchasesAwaitingGemTable.IsEmpty())
	{
		return;
	}

	TArray<UMGAndroidPurchase*> Waiting;
	for (UMGAndroidPurchase* Purchase : PurchasesAwaitingGemTable)
	{
		Waiting.Add(Purchase);
	}
	PurchasesAwaitingGemTable.Reset();

	ProcessPurchases(Waiting);
}

void UStoreSubsystem::GrantRemoveAds()
{
	USaveSubsystem* Save = GetGameInstance()->GetSubsystem<USaveSubsystem>();
	if (!Save || Save->GetProgression().bNoAds)
	{
		return;
	}

	UE_LOG(LogFlappyHoop, Log, TEXT("[Billing] Remove ads granted"));

	FUserProgression Updated = Save->GetProgression();
	Updated.bNoAds = true;
	Save->Save(Updated);

	if (UAdsSubsystem* Ads = GetGameInstance()->GetSubsystem<UAdsSubsystem>())
	{
		Ads->HandleAdsRemoved();
	}

	OnAdsRemoved.Broadcast();
}

void UStoreSubsystem::AcknowledgeNext()
{
#if PLATFORM_ANDROID
	if (bAcknowledgeInFlight || AcknowledgeQueue.IsEmpty() || !IsReady())
	{
		return;
	}

	bAcknowledgeInFlight = true;
	const FString Token = AcknowledgeQueue[0];
	AcknowledgeQueue.RemoveAt(0);

	OnAcknowledgeCompletedDelegate.BindDynamic(this, &UStoreSubsystem::OnAcknowledgeCompleted);
	BillingClient->AcknowledgePurchase(Token, OnAcknowledgeCompletedDelegate);
#endif
}

void UStoreSubsystem::OnAcknowledgeCompleted(UMGAndroidBillingResult* Result)
{
	// If this fails, the purchase stays unacknowledged and is retried by QueryOwnedPurchases on next launch.
	UFunctionsLibrary::BillingResponseOK(Result, TEXT("Acknowledge"));

	bAcknowledgeInFlight = false;
	AcknowledgeNext();
}

void UStoreSubsystem::ConsumeNext()
{
#if PLATFORM_ANDROID
	if (bConsumeInFlight || ConsumeQueue.IsEmpty() || !IsReady())
	{
		return;
	}

	bConsumeInFlight = true;
	CurrentConsume = ConsumeQueue[0];
	ConsumeQueue.RemoveAt(0);

	OnConsumeCompletedDelegate.BindDynamic(this, &UStoreSubsystem::OnConsumeCompleted);
	BillingClient->Consume(CurrentConsume.Token, OnConsumeCompletedDelegate);
#endif
}

void UStoreSubsystem::OnConsumeCompleted(UMGAndroidBillingResult* Result, const FString& Token)
{
	bConsumeInFlight = false;

	if (!UFunctionsLibrary::BillingResponseOK(Result, TEXT("Consume")))
	{
		// Allow a retry, and re-query now so the paid gems aren't delayed until the next launch.
		HandledPurchaseTokens.Remove(CurrentConsume.Token);
		CurrentConsume = FFlappyPendingConsume();
		QueryOwnedPurchases();
		return;
	}

	const int32* Amount = GemProductAmounts.Find(CurrentConsume.ProductId);
	if (Amount && *Amount > 0)
	{
		UE_LOG(LogFlappyHoop, Log, TEXT("[Billing] +%d gems (%s)"), *Amount, *CurrentConsume.ProductId);

		if (UEconomySubsystem* Economy = GetGameInstance()->GetSubsystem<UEconomySubsystem>())
		{
			Economy->AddGems(*Amount); // saves + broadcasts OnBalanceChanged
		}
		else
		{
			UE_LOG(LogFlappyHoop, Error, TEXT("[Billing] Economy subsystem missing; %d gems NOT credited"), *Amount);
		}
	}

	CurrentConsume = FFlappyPendingConsume();
	ConsumeNext();
}
