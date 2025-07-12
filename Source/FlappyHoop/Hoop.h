// Hoop.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Hoop.generated.h"

UCLASS()
class FLAPPYHOOP_API AHoop : public AActor
{
    GENERATED_BODY()

public:
    AHoop();

protected:

    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* HoopMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* HoopRing;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* ScoreCylinder;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* RimDetector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    float SwitchSidesDelay = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    class USoundBase* ScoreSound;

    //minimum height difference required for valid scoring
    UPROPERTY(EditDefaultsOnly, Category = "Settings")
    float MinimumHeightDifference = 50.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Settings")
    float MinimumDownwardVelocity = 0.0f;

public:
    virtual void Tick(float DeltaTime) override;

    UFUNCTION(BlueprintCallable, Category = "Hoop")
    void SetRingRotation(bool bRightEdgeHoop);

private:

    UFUNCTION()
    void OnScoreCylinderBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnRimDetectorBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void SwitchSides();

    bool IsValidScoringAttempt(AActor* Ball);

    TSet<AActor*> BallsAboveRim;
    TMap<AActor*, float> BallMaxHeights;

    UWorld* World;
    class IBallInterface* BallInterface;
    class IGameModeInterface* GameModeInterface;
    class AMyPlayerController* PlayerController;
    void ResetBallTracking();
};