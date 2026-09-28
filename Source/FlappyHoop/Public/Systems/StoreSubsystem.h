// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MGAndroidBillingLibrary.h"
#include "Engine/TimerHandle.h"
#include "StoreSubsystem.generated.h"

class FTimerManager;
class UMGAndroidBillingClient;
class UMGAndroidBillingResult;
class UMGAndroidPurchase;
class UMGAndroidProductDetails;

namespace FlappyHoopStore
{
	/** Non-consumable Play Store product that removes ads. */
	inline const TCHAR* RemoveAdsProductId = TEXT("removeads");
}

DECLARE_MULTICAST_DELEGATE(FOnAdsRemoved);

/** Fired when localized prices arrive from Google Play. */
DECLARE_MULTICAST_DELEGATE(FOnStorePricesUpdated);

/** A purchased gem pack waiting to be consumed (plain C++ struct, not reflected). */
struct FFlappyPendingConsume
{
	FString Token;
	FString ProductId;
};

/**
 * Google Play Billing (Monetization Goodies, Play Billing 8): connection with reconnect/backoff,
 * purchase flow, restoring "remove ads", and consuming gem packs (credited through UEconomySubsystem).
 * On non-Android platforms everything is a no-op.
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

	/** Opens the Play Store purchase sheet for a product. The result arrives through the purchase listener. */
	void Purchase(const FString& ProductId);

	/** Fired when "remove ads" is bought or restored. */
	FOnAdsRemoved OnAdsRemoved;

	/**
	 * The price Google Play shows for a product, in the player's currency (e.g. "€0,99").
	 * Empty until the store has answered (and always on non-Android platforms): show your own fallback then.
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
};
