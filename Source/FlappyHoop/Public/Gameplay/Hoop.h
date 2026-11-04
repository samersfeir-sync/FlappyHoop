// Hoop.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/TimerHandle.h"
#include "Hoop.generated.h"

class UStaticMeshComponent;
class USoundBase;
class IBallInterface;
class ABaseGameMode;

UCLASS()
class FLAPPYHOOP_API AHoop : public AActor
{
    GENERATED_BODY()

public:

    AHoop();

    UFUNCTION(BlueprintCallable, Category = "Hoop")
    void SetRingRotation(bool bRightEdgeHoop);

    /**
     * Placed at a new height: drifts up and down around BaseZ (amount/speed from the difficulty level),
     * never leaving [MinZ, MaxZ]. Does nothing while the current level has no drift.
     */
    void StartDrift(double BaseZ, double MinZ, double MaxZ);
    void StopDrift();

protected:

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> HoopMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> HoopRing;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> ScoreCylinder;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> RimDetector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    float SwitchSidesDelay = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TObjectPtr<USoundBase> ScoreSound;

    /** Optional. Played instead of ScoreSound on a swish. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    TObjectPtr<USoundBase> SwishSound;

    /**
     * A basket only counts if the ball reaches ScoreCylinder within this many seconds of dropping into RimDetector
     * (one continuous drop). Stops an old near-miss from making a later touch from the side/below count.
     */
    UPROPERTY(EditDefaultsOnly, Category = "Settings", meta = (ClampMin = "0.05", Units = "s"))
    float RimToScoreMaxTime = 0.75f;

    /** Logs why each ScoreCylinder touch was accepted or rejected (Output Log, "Basket"). */
    UPROPERTY(EditAnywhere, Category = "Settings")
    bool bLogScoringDecisions = false;

    /** Ball must be falling faster than this (cm/s, positive number) when entering the rim and scoring. */
    UPROPERTY(EditDefaultsOnly, Category = "Settings", meta = (ClampMin = "0.0"))
    float MinimumDownwardVelocity = 0.0f;

    /** Any rim/backboard contact within this many seconds before entering the rim disqualifies a swish. */
    UPROPERTY(EditDefaultsOnly, Category = "Settings|Swish", meta = (ClampMin = "0.0"))
    float SwishContactWindow = 0.35f;

private:

    UFUNCTION()
    void OnScoreCylinderBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnScoreCylinderEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    UFUNCTION()
    void OnRimDetectorBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnRimDetectorEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    UFUNCTION()
    void OnHoopPartHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
        FVector NormalImpulse, const FHitResult& Hit);

    UFUNCTION()
    void SwitchSides();

    bool IsValidScoringAttempt(AActor* Ball, FString& OutReason) const;

    /** Validates and awards a ScoreCylinder touch. Rejections are logged/acted on only if bFromOverlapEvent. */
    void TryScore(AActor* Ball, bool bFromOverlapEvent);

    /** Marks Ball as "coming down through the rim" (or refreshes the mark) if it's inside RimDetector, above the score zone and falling relative to the hoop. */
    void TryMarkRimEntry(AActor* Ball);

    /** Ball's vertical velocity relative to the (possibly drifting) hoop. */
    double GetRelativeVelocityZ(const AActor* Ball) const;

    void ResetBallTracking();

    /** A ball that dropped into RimDetector: when, and where its centre was. */
    struct FRimEntry
    {
        /** Last time the drop was confirmed (refreshed while the ball stays in the rim falling). */
        double Time = 0.0;
        /** First time it dropped in (for swish detection). */
        double FirstTime = 0.0;
        /** Ball centre height above ScoreCylinder's centre when last confirmed (relative, so hoop drift doesn't matter). */
        double HeightAboveZone = 0.0;
    };

    /** The ball's drop mark, or null if it has none or it's older than RimToScoreMaxTime. */
    const FRimEntry* FindFreshRimEntry(const AActor* Ball) const;

    /**
     * Balls currently "coming down through the rim". Removed when they score, leave RimDetector going up,
     * or touch ScoreCylinder while rising; ignored once older than RimToScoreMaxTime.
     * Weak keys so a destroyed actor can't leave a dangling entry.
     */
    TMap<TWeakObjectPtr<AActor>, FRimEntry> RimEntries;

    /** Balls currently overlapping RimDetector. */
    TSet<TWeakObjectPtr<AActor>> BallsInRim;

    /** Balls that touched ScoreCylinder going up (came from below): can't get a rim mark until they're clear of the hoop. */
    TSet<TWeakObjectPtr<AActor>> CameFromBelow;

    /** Hoop's vertical speed this frame (cm/s), from the drift. */
    double HoopVelocityZ = 0.0;
    bool bDriftActive = false;

    /** World time of the last rim/backboard contact (for swish detection). */
    double LastRimContactTime = -1000.0;
    bool bPendingSwish = false;

    TWeakObjectPtr<AActor> ScoringBall;
    UPROPERTY(Transient)
    TObjectPtr<ABaseGameMode> GameMode = nullptr;

    FTimerHandle SwitchSidesTimer;

    // --- Drift (Hard mode) ---
    double DriftBaseZ = 0.0;
    double DriftMinZ = 0.0;
    double DriftMaxZ = 0.0;
    /** Phase in cycles; advanced by the level's speed so speed changes don't make the hoop jump. */
    double DriftPhase = 0.0;
    /** +1 or -1: whether this placement starts drifting up or down. */
    double DriftSign = 1.0;
};
