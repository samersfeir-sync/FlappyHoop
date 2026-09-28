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

    /** Minimum height (cm) the ball must have been above the score cylinder for a valid basket. */
    UPROPERTY(EditDefaultsOnly, Category = "Settings")
    float MinimumHeightDifference = 50.0f;

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
    void OnHoopPartHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
        FVector NormalImpulse, const FHitResult& Hit);

    UFUNCTION()
    void SwitchSides();

    bool IsValidScoringAttempt(AActor* Ball) const;

    void ResetBallTracking();

    /** Per-ball tracking of the current shot. Weak so a destroyed actor can't leave a dangling key. */
    TSet<TWeakObjectPtr<AActor>> BallsAboveRim;
    TMap<TWeakObjectPtr<AActor>, float> BallMaxHeights;

    /** World time of the last rim/backboard contact and of entering the rim detector (for swish detection). */
    double LastRimContactTime = -1000.0;
    double RimEntryTime = 0.0;
    bool bPendingSwish = false;

    TWeakObjectPtr<AActor> ScoringBall;
    UPROPERTY(Transient)
    TObjectPtr<ABaseGameMode> GameMode = nullptr;

    FTimerHandle SwitchSidesTimer;
};
