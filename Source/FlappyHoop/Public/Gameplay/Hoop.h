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

protected:

    virtual void BeginPlay() override;

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

    void ResetBallTracking();

    /** A ball that dropped into RimDetector: when, and where its centre was. */
    struct FRimEntry
    {
        double Time = 0.0;
        double BallZ = 0.0;
    };

    /**
     * Balls currently "coming down through the rim". Removed when they score, leave RimDetector going up,
     * or touch ScoreCylinder while rising; ignored once older than RimToScoreMaxTime.
     * Weak keys so a destroyed actor can't leave a dangling entry.
     */
    TMap<TWeakObjectPtr<AActor>, FRimEntry> RimEntries;

    /** World time of the last rim/backboard contact (for swish detection). */
    double LastRimContactTime = -1000.0;
    bool bPendingSwish = false;

    TWeakObjectPtr<AActor> ScoringBall;
    UPROPERTY(Transient)
    TObjectPtr<ABaseGameMode> GameMode = nullptr;

    FTimerHandle SwitchSidesTimer;
};
