// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MGAndroidBillingLibrary.h"
#include "StoreKit2/MGIosStoreKit2StoreKitManager.h"
#include "Engine/TimerHandle.h"
#include "StoreSubsystem.generated.h"

class FTimerManager;
class UMGAndroidBillingClient;
class UMGAndroidBillingResult;
class UMGAndroidPurchase;
class UMGAndroidProductDetails;
class UMGIosStoreKit2StoreKitManager;
class UMGIosStoreKit2Product;
class UMGIosStoreKit2Transaction;

namespace FlappyHoopStore
{
	/** Non-consumable product that removes ads (same ID on Google Play and the App Store). */
	inline const TCHAR* RemoveAdsProductId = TEXT("removeads");
}

DECLARE_MULTICAST_DELEGATE(FOnAdsRemoved);

/** Fired when localized prices arrive from Google Play / the App Store. */
DECLARE_MULTICAST_DELEGATE(FOnStorePricesUpdated);

/** Fired when a player-initiated restore finishes. bAdsRemoved: "remove ads" is owned after the restore. */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPurchasesRestored, bool /*bAdsRemoved*/);

/** A purchased gem pack waiting to be consumed (plain C++ struct, not reflected). */
struct FFlappyPendingConsume
{
	FString Token;
	FString ProductId;
};

/**
 * In-app purchases through Monetization Goodies:
 * - Android: Google Play Billing 8 (connection with reconnect/backoff, ack/consume queues).
 * - iOS: StoreKit 2 (verified transactions, finished after the content is granted).
 * Both: purchase flow, restoring "remove ads", gem packs credited through UEconomySubsystem, localized prices.
 * Product IDs are shared: create the same IDs in Play Console and App Store Connect.
 * On other platforms everything is a no-op.
 */
UCLASS()
class FLAPPYHOOP_API UStoreSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	static UStoreSubsystem* Get(const UObject* WorldContextObject);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	bool IsReady() const;

	/** Tell the store which product IDs are consumable gem packs and how many gems each grants. */
	void RegisterGemProducts(const TMap<FString, int32>& ProductIdToGemAmount);

	/** Opens the store's purchase sheet for a product. The result arrives through the purchase listener. */
	void Purchase(const FString& ProductId);

	/**
	 * Player-initiated "Restore Purchases" (App Store Review requires a button for it on iOS).
	 * On Android this re-queries owned purchases (which also happens automatically on every connect).
	 */
	void RestorePurchases();

	/** True on platforms where a visible "Restore Purchases" button is expected (iOS). */
	static bool ShouldShowRestoreButton();

	/**
	 * Fired when a RestorePurchases() call finishes. On Android the re-query result is asynchronous and silent,
	 * so it fires right away with the current "remove ads" state; a restored purchase still fires OnAdsRemoved.
	 */
	FOnPurchasesRestored OnPurchasesRestored;

	/** Fired when "remove ads" is bought or restored. */
	FOnAdsRemoved OnAdsRemoved;

	/**
	 * The price the store shows for a product, in the player's currency (e.g. "€0,99").
	 * Empty until the store has answered (and always on desktop/editor): show your own fallback then.
	 */
	FString GetLocalizedPrice(const FString& ProductId) const;

	/** Fired when localized prices arrive or change. */
	FOnStorePricesUpdated OnPricesUpdated;

private:

	FTimerManager* GetTimers() const;

	UPROPERTY(Transient)
	TObjectPtr<UMGAndroidBillingClient> BillingClient = nullptr;

	FOnPurchasesUpdated OnPurchasesUpdatedDelegate;
	FAndroidBillingResultDelegate OnBillingSetupFinishedDelegate;
	FAndroidBillingVoidDelegate OnBillingDisconnectedDelegate;
	FAndroidPurchasesDelegate OnQueryPurchasesDelegate;
	FAndroidBillingResultDelegate OnAcknowledgeCompletedDelegate;
	FAndroidConsumeResultDelegate OnConsumeCompletedDelegate;
	FAndroidProductDetailsDelegate OnProductDetailsDelegate;
	FAndroidBillingResultDelegate OnBillingFlowStartedDelegate;

	// --- Connection ---
	void StartConnection();

	UFUNCTION()
	void OnBillingSetupFinished(UMGAndroidBillingResult* Result);

	UFUNCTION()
	void OnBillingDisconnected();

	FTimerHandle ReconnectTimer;
	int32 ReconnectAttempts = 0;

	// --- Product details / prices ---
	/**
	 * The plugin keeps the product-details callback in ONE static delegate, so only one query may be in flight.
	 * Price queries and purchase queries are therefore serialized here.
	 */
	bool bProductQueryInFlight = false;
	bool bPriceQueryPending = false;
	FString PendingPurchaseId;

	/** Asks Google Play for the details (prices) of the gem packs and "remove ads". */
	void QueryPrices();

	UFUNCTION()
	void OnPricesQueried(UMGAndroidBillingResult* Result, const TArray<UMGAndroidProductDetails*>& ProductDetails);

	/** Runs whatever product work was waiting for the in-flight query (a purchase first, then prices). */
	void RunPendingProductWork();

	void CacheProductDetails(const TArray<UMGAndroidProductDetails*>& ProductDetails);

	/** Details kept from the last queries, so a purchase can open the Play sheet without another round trip. */
	UPROPERTY(Transient)
	TMap<FString, TObjectPtr<UMGAndroidProductDetails>> CachedProductDetails;

	TMap<FString, FString> LocalizedPrices;

	FAndroidProductDetailsDelegate OnPricesQueriedDelegate;

	// --- Purchase flow ---
	void LaunchPurchase(UMGAndroidProductDetails* Details);

	UFUNCTION()
	void OnProductDetailsQueried(UMGAndroidBillingResult* Result, const TArray<UMGAndroidProductDetails*>& ProductDetails);

	UFUNCTION()
	void OnBillingFlowStarted(UMGAndroidBillingResult* Result);

	bool bPurchaseFlowInProgress = false;

	// --- Owned purchases ---
	/** Re-queries owned purchases; restores "remove ads" and finishes gem packs that were paid but not consumed. */
	void QueryOwnedPurchases();

	UFUNCTION()
	void OnPurchasesUpdated(UMGAndroidBillingResult* Result, const TArray<UMGAndroidPurchase*>& Purchases);

	UFUNCTION()
	void OnOwnedPurchasesQueried(UMGAndroidBillingResult* Result, const TArray<UMGAndroidPurchase*>& Purchases);

	void ProcessPurchases(const TArray<UMGAndroidPurchase*>& Purchases);
	void ProcessPurchase(UMGAndroidPurchase* Purchase);
	void GrantRemoveAds();

	/**
	 * The plugin stores ack/consume callbacks in single static delegates,
	 * so only one of each can be in flight. Work is queued and run one at a time.
	 */
	TArray<FString> AcknowledgeQueue;
	bool bAcknowledgeInFlight = false;
	void AcknowledgeNext();

	UFUNCTION()
	void OnAcknowledgeCompleted(UMGAndroidBillingResult* Result);

	TArray<FFlappyPendingConsume> ConsumeQueue;
	bool bConsumeInFlight = false;
	FFlappyPendingConsume CurrentConsume;
	void ConsumeNext();

	UFUNCTION()
	void OnConsumeCompleted(UMGAndroidBillingResult* Result, const FString& Token);

	/** Tokens already queued/handled this session, so a purchase reported twice isn't credited twice. */
	TSet<FString> HandledPurchaseTokens;

	/** Gem purchases that arrived before the shop registered gem amounts. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMGAndroidPurchase>> PurchasesAwaitingGemTable;

	TMap<FString, int32> GemProductAmounts;

	/** Credits a gem pack (Android: after consume; iOS: before finishing the transaction). */
	void CreditGems(const FString& ProductId);

	// =================================================================================================================
	// iOS (StoreKit 2)
	// =================================================================================================================

	void InitializeIos();

	/** The plugin's shared manager (rooted by the plugin, kept here for convenience). */
	UPROPERTY(Transient)
	TObjectPtr<UMGIosStoreKit2StoreKitManager> IosStore = nullptr;

	/** Products fetched from the App Store, by product ID (needed to start a purchase). */
	UPROPERTY(Transient)
	TMap<FString, TObjectPtr<UMGIosStoreKit2Product>> IosProducts;

	/** Gem transactions that arrived before the shop registered gem amounts (not finished yet). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMGIosStoreKit2Transaction>> IosTransactionsAwaitingGemTable;

	/** Transaction IDs already granted this session (purchase result and the updates listener can both report one). */
	TSet<int64> HandledIosTransactions;

	/** Product the player tapped while its App Store details were still being fetched. */
	FString PendingIosPurchaseId;

	bool bIosRestoreFromPlayer = false;

	FMGStoreKit2TransactionDelegate IosTransactionUpdatedDelegate;
	FMGStoreKit2ProductsSuccessDelegate IosPricesFetchedDelegate;
	FMGStoreKit2ProductsSuccessDelegate IosPurchaseProductFetchedDelegate;
	FMGStoreKit2PurchaseSuccessDelegate IosPurchaseSucceededDelegate;
	FMGStoreKit2ErrorDelegate IosPurchaseFailedDelegate;
	FMGStoreKit2ErrorDelegate IosFetchFailedDelegate;
	FMGStoreKit2RestorePurchasesSuccessDelegate IosRestoreDelegate;

	void PurchaseIos(const FString& ProductId);
	void LaunchIosPurchase(UMGIosStoreKit2Product* Product);
	void QueryPricesIos();
	void RestoreIos(bool bFromPlayer);
	void CacheIosProducts(const TArray<UMGIosStoreKit2Product*>& Products);

	/** Grants what a verified transaction paid for, then finishes it. Safe to call twice for the same transaction. */
	void HandleIosTransaction(UMGIosStoreKit2Transaction* Transaction);

	UFUNCTION()
	void OnIosTransactionUpdated(UMGIosStoreKit2Transaction* Transaction);

	UFUNCTION()
	void OnIosPricesFetched(const TArray<UMGIosStoreKit2Product*>& Products);

	UFUNCTION()
	void OnIosPurchaseProductFetched(const TArray<UMGIosStoreKit2Product*>& Products);

	UFUNCTION()
	void OnIosFetchFailed(int32 ErrorCode, const FString& ErrorMessage);

	UFUNCTION()
	void OnIosPurchaseSucceeded(UMGIosStoreKit2Transaction* Transaction);

	UFUNCTION()
	void OnIosPurchaseFailed(int32 ErrorCode, const FString& ErrorMessage);

	UFUNCTION()
	void OnIosRestored(const TArray<UMGIosStoreKit2Transaction*>& Transactions);
};
