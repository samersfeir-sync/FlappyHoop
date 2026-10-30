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
#include "StoreKit2/MGIosStoreKit2StoreKitManager.h"
#include "StoreKit2/MGIosStoreKit2Product.h"
#include "StoreKit2/MGIosStoreKit2Transaction.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "TimerManager.h"

namespace
{
	/**
	 * UMGIosWrapperObject::IsNativeObjectValid isn't exported from the plugin DLL, so calling it breaks the
	 * Win64 editor link. iOS builds are monolithic (static link), so it's only called there.
	 */
	bool IsIosObjectValid(UMGIosWrapperObject* Object)
	{
#if PLATFORM_IOS
		return Object && Object->IsNativeObjectValid();
#else
		return Object != nullptr;
#endif
	}
}

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
#elif PLATFORM_IOS
	InitializeIos();
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
#elif PLATFORM_IOS
	// The plugin's manager is a rooted singleton: stop it from calling into this subsystem.
	if (IosStore)
	{
		IosStore->SetOnTransactionUpdateDelegate(FMGStoreKit2TransactionDelegate());
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
#elif PLATFORM_IOS
	return IosStore && IosStore->IsNativeObjectValid();
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
#elif PLATFORM_IOS
	PurchaseIos(ProductId);
#else
	UE_LOG(LogFlappyHoop, Log, TEXT("[Billing] Purchase(%s) ignored: no store on this platform"), *ProductId);
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
#if PLATFORM_IOS
	QueryPricesIos();
#elif PLATFORM_ANDROID
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
#if PLATFORM_ANDROID
	if (!IsReady())
	{
		return;
	}

	// Play Billing 8 removed queryPurchaseHistory. queryPurchases returns everything currently owned,
	// which is exactly what we need to restore "remove ads" and finish unconsumed gem packs.
	OnQueryPurchasesDelegate.BindDynamic(this, &UStoreSubsystem::OnOwnedPurchasesQueried);
	BillingClient->QueryPurchases(EMGSkuType::Purchase, OnQueryPurchasesDelegate);
#endif
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

	if (!IosTransactionsAwaitingGemTable.IsEmpty())
	{
		TArray<UMGIosStoreKit2Transaction*> WaitingIos;
		for (UMGIosStoreKit2Transaction* Transaction : IosTransactionsAwaitingGemTable)
		{
			WaitingIos.Add(Transaction);
		}
		IosTransactionsAwaitingGemTable.Reset();

		for (UMGIosStoreKit2Transaction* Transaction : WaitingIos)
		{
			HandleIosTransaction(Transaction);
		}
	}

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

	CreditGems(CurrentConsume.ProductId);

	CurrentConsume = FFlappyPendingConsume();
	ConsumeNext();
}

void UStoreSubsystem::CreditGems(const FString& ProductId)
{
	const int32* Amount = GemProductAmounts.Find(ProductId);
	if (!Amount || *Amount <= 0)
	{
		return;
	}

	UE_LOG(LogFlappyHoop, Log, TEXT("[Billing] +%d gems (%s)"), *Amount, *ProductId);

	if (UEconomySubsystem* Economy = GetGameInstance()->GetSubsystem<UEconomySubsystem>())
	{
		Economy->AddGems(*Amount); // saves + broadcasts OnBalanceChanged
	}
	else
	{
		UE_LOG(LogFlappyHoop, Error, TEXT("[Billing] Economy subsystem missing; %d gems NOT credited"), *Amount);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// Restore (player-initiated)
// ---------------------------------------------------------------------------------------------------------------------

bool UStoreSubsystem::ShouldShowRestoreButton()
{
#if PLATFORM_IOS
	return true;
#else
	return false;
#endif
}

void UStoreSubsystem::RestorePurchases()
{
#if PLATFORM_IOS
	RestoreIos(true);
#else
	QueryOwnedPurchases(); // Android: no-op when not connected; it runs on every connect anyway
	const USaveSubsystem* Save = GetGameInstance()->GetSubsystem<USaveSubsystem>();
	OnPurchasesRestored.Broadcast(Save && Save->GetProgression().bNoAds);
#endif
}

// =====================================================================================================================
// iOS (StoreKit 2)
//
// Flow: the plugin's shared manager listens to Transaction.updates (verified transactions only). Unfinished
// transactions (e.g. app killed after paying) are delivered there on launch, as are Ask-to-Buy approvals and
// purchases made outside the app. A purchase made in the app returns its transaction to OnIosPurchaseSucceeded.
// Every transaction goes through HandleIosTransaction: grant the content, then Finish() it. Consumables
// (gem packs) stay "unfinished" until granted, so a crash before granting re-delivers them on next launch.
// =====================================================================================================================

void UStoreSubsystem::InitializeIos()
{
	IosStore = UMGIosStoreKit2StoreKitManager::StoreKit2GetSharedInstance();
	if (!IsReady())
	{
		UE_LOG(LogFlappyHoop, Error, TEXT("[Billing] StoreKit 2 manager could not be created"));
		return;
	}

	// Bind right away: queued transactions are delivered on the game thread after this call returns.
	IosTransactionUpdatedDelegate.BindDynamic(this, &UStoreSubsystem::OnIosTransactionUpdated);
	IosStore->SetOnTransactionUpdateDelegate(IosTransactionUpdatedDelegate);

	IosPricesFetchedDelegate.BindDynamic(this, &UStoreSubsystem::OnIosPricesFetched);
	IosPurchaseProductFetchedDelegate.BindDynamic(this, &UStoreSubsystem::OnIosPurchaseProductFetched);
	IosFetchFailedDelegate.BindDynamic(this, &UStoreSubsystem::OnIosFetchFailed);
	IosPurchaseSucceededDelegate.BindDynamic(this, &UStoreSubsystem::OnIosPurchaseSucceeded);
	IosPurchaseFailedDelegate.BindDynamic(this, &UStoreSubsystem::OnIosPurchaseFailed);
	IosRestoreDelegate.BindDynamic(this, &UStoreSubsystem::OnIosRestored);

	// Silent restore of "remove ads" (Transaction.currentEntitlements; never shows a sign-in prompt).
	RestoreIos(false);
	QueryPricesIos();
}

void UStoreSubsystem::PurchaseIos(const FString& ProductId)
{
	if (bPurchaseFlowInProgress || ProductId.IsEmpty() || !IsReady())
	{
		UE_LOG(LogFlappyHoop, Warning, TEXT("[Billing] Can't start purchase for %s (busy or store unavailable)"), *ProductId);
		return;
	}

	bPurchaseFlowInProgress = true;

	if (const TObjectPtr<UMGIosStoreKit2Product>* Cached = IosProducts.Find(ProductId))
	{
		UMGIosStoreKit2Product* Product = Cached->Get();
		if (IsIosObjectValid(Product))
		{
			LaunchIosPurchase(Product);
			return;
		}
	}

	// Not fetched yet (prices not loaded / offline earlier): fetch it, then buy.
	PendingIosPurchaseId = ProductId;
	IosStore->FetchProducts({ ProductId }, IosPurchaseProductFetchedDelegate, IosFetchFailedDelegate);
}

void UStoreSubsystem::OnIosPurchaseProductFetched(const TArray<UMGIosStoreKit2Product*>& Products)
{
	CacheIosProducts(Products);

	const FString Id = PendingIosPurchaseId;
	PendingIosPurchaseId.Reset();

	const TObjectPtr<UMGIosStoreKit2Product>* Found = IosProducts.Find(Id);
	if (Id.IsEmpty() || !Found || !Found->Get())
	{
		UE_LOG(LogFlappyHoop, Warning, TEXT("[Billing] App Store doesn't know product '%s' (check App Store Connect / Paid Apps agreement)"), *Id);
		bPurchaseFlowInProgress = false;
		return;
	}

	LaunchIosPurchase(Found->Get());
}

void UStoreSubsystem::OnIosFetchFailed(int32 ErrorCode, const FString& ErrorMessage)
{
	UE_LOG(LogFlappyHoop, Warning, TEXT("[Billing] FetchProducts failed: %d %s"), ErrorCode, *ErrorMessage);
	PendingIosPurchaseId.Reset();
	bPurchaseFlowInProgress = false;
}

void UStoreSubsystem::LaunchIosPurchase(UMGIosStoreKit2Product* Product)
{
	if (!Product || !IsReady())
	{
		bPurchaseFlowInProgress = false;
		return;
	}

	UE_LOG(LogFlappyHoop, Log, TEXT("[Billing] Purchasing %s"), *Product->GetProductId());
	IosStore->PurchaseProduct(Product, IosPurchaseSucceededDelegate, IosPurchaseFailedDelegate);
}

void UStoreSubsystem::OnIosPurchaseSucceeded(UMGIosStoreKit2Transaction* Transaction)
{
	bPurchaseFlowInProgress = false;
	HandleIosTransaction(Transaction);
}

void UStoreSubsystem::OnIosPurchaseFailed(int32 ErrorCode, const FString& ErrorMessage)
{
	// Also reached when the player cancels, or when the purchase is pending (Ask to Buy / SCA):
	// a pending purchase that is approved later arrives through OnIosTransactionUpdated.
	UE_LOG(LogFlappyHoop, Log, TEXT("[Billing] Purchase not completed: %d %s"), ErrorCode, *ErrorMessage);
	bPurchaseFlowInProgress = false;
}

void UStoreSubsystem::OnIosTransactionUpdated(UMGIosStoreKit2Transaction* Transaction)
{
	HandleIosTransaction(Transaction);
}

void UStoreSubsystem::HandleIosTransaction(UMGIosStoreKit2Transaction* Transaction)
{
	if (!IsIosObjectValid(Transaction))
	{
		return;
	}

	const FString ProductId = Transaction->GetProductId();
	const int64 TransactionId = Transaction->GetTransactionId();

	// Refunded / revoked: nothing to grant. ("Remove ads" stays granted once saved, same as on Android.)
	if (Transaction->GetRevocationDate() != FDateTime())
	{
		UE_LOG(LogFlappyHoop, Log, TEXT("[Billing] Transaction %lld (%s) was revoked"), TransactionId, *ProductId);
		Transaction->Finish();
		return;
	}

	if (HandledIosTransactions.Contains(TransactionId))
	{
		Transaction->Finish(); // harmless if already finished
		return;
	}

	if (ProductId == FlappyHoopStore::RemoveAdsProductId)
	{
		HandledIosTransactions.Add(TransactionId);
		GrantRemoveAds();
		Transaction->Finish();
		return;
	}

	if (GemProductAmounts.Contains(ProductId))
	{
		HandledIosTransactions.Add(TransactionId);
		CreditGems(ProductId);
		Transaction->Finish(); // only after crediting, so a crash before this re-delivers it
		return;
	}

	// Most likely a gem pack delivered at launch before the shop registered the gem table. Keep it unfinished.
	IosTransactionsAwaitingGemTable.AddUnique(Transaction);
}

void UStoreSubsystem::QueryPricesIos()
{
	if (!IsReady())
	{
		return;
	}

	TArray<FString> ProductIds;
	GemProductAmounts.GetKeys(ProductIds);
	ProductIds.AddUnique(FlappyHoopStore::RemoveAdsProductId);

	// The plugin copies the delegates per call, so several fetches may be in flight at once.
	IosStore->FetchProducts(ProductIds, IosPricesFetchedDelegate, IosFetchFailedDelegate);
}

void UStoreSubsystem::OnIosPricesFetched(const TArray<UMGIosStoreKit2Product*>& Products)
{
	CacheIosProducts(Products);
	OnPricesUpdated.Broadcast();
}

void UStoreSubsystem::CacheIosProducts(const TArray<UMGIosStoreKit2Product*>& Products)
{
	for (UMGIosStoreKit2Product* Product : Products)
	{
		if (!IsIosObjectValid(Product))
		{
			continue;
		}

		const FString Id = Product->GetProductId();
		if (Id.IsEmpty())
		{
			continue;
		}

		IosProducts.Add(Id, Product);

		const FString Price = Product->GetDisplayPrice();
		if (!Price.IsEmpty())
		{
			LocalizedPrices.Add(Id, Price);
			UE_LOG(LogFlappyHoop, Log, TEXT("[Billing] Price %s = %s"), *Id, *Price);
		}
	}
}

void UStoreSubsystem::RestoreIos(bool bFromPlayer)
{
	if (!IsReady())
	{
		if (bFromPlayer)
		{
			OnPurchasesRestored.Broadcast(false);
		}
		return;
	}

	bIosRestoreFromPlayer |= bFromPlayer;
	IosStore->RestorePurchases(IosRestoreDelegate);
}

void UStoreSubsystem::OnIosRestored(const TArray<UMGIosStoreKit2Transaction*>& Transactions)
{
	// currentEntitlements: non-consumables the player owns ("remove ads"). Gem packs never appear here.
	for (UMGIosStoreKit2Transaction* Transaction : Transactions)
	{
		HandleIosTransaction(Transaction);
	}

	if (bIosRestoreFromPlayer)
	{
		bIosRestoreFromPlayer = false;
		const USaveSubsystem* Save = GetGameInstance()->GetSubsystem<USaveSubsystem>();
		OnPurchasesRestored.Broadcast(Save && Save->GetProgression().bNoAds);
	}
}
