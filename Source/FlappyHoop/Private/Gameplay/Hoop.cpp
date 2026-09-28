// Fill out your copyright notice in the Description page of Project Settings.

#include "Gameplay/Hoop.h"
#include "Core/FlappyHoop.h"
#include "Gameplay/BallInterface.h"
#include "Framework/BaseGameMode.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AHoop::AHoop()
{
    PrimaryActorTick.bCanEverTick = false;

    HoopMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HoopMesh"));
    RootComponent = HoopMesh;

    HoopRing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HoopRing"));
    HoopRing->SetupAttachment(HoopMesh);

    ScoreCylinder = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ScoreCylinder"));
    ScoreCylinder->SetupAttachment(HoopRing);
    ScoreCylinder->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    ScoreCylinder->SetCollisionResponseToAllChannels(ECR_Overlap);
    ScoreCylinder->SetGenerateOverlapEvents(true);

    RimDetector = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RimDetector"));
    RimDetector->SetupAttachment(HoopRing);
    RimDetector->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    RimDetector->SetCollisionResponseToAllChannels(ECR_Overlap);
    RimDetector->SetGenerateOverlapEvents(true);
}

void AHoop::BeginPlay()
{
    Super::BeginPlay();

    ScoreCylinder->OnComponentBeginOverlap.AddUniqueDynamic(this, &AHoop::OnScoreCylinderBeginOverlap);
    RimDetector->OnComponentBeginOverlap.AddUniqueDynamic(this, &AHoop::OnRimDetectorBeginOverlap);

    // Blocking hits from the ball's projectile sweep are dispatched to these components too (same as Ground's OnActorHit).
    HoopMesh->OnComponentHit.AddUniqueDynamic(this, &AHoop::OnHoopPartHit);
    HoopRing->OnComponentHit.AddUniqueDynamic(this, &AHoop::OnHoopPartHit);

    GameMode = ABaseGameMode::Get(this);
    if (GameMode)
    {
        GameMode->OnGameReset.AddUObject(this, &AHoop::ResetBallTracking);
    }

}

void AHoop::SetRingRotation(bool bRightEdgeHoop)
{
    FRotator Rotation = HoopRing->GetRelativeRotation();
    Rotation.Pitch = bRightEdgeHoop ? -10.0f : 10.0f;
    HoopRing->SetRelativeRotation(Rotation);
}

void AHoop::OnHoopPartHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
    FVector NormalImpulse, const FHitResult& Hit)
{
    if (OtherActor && OtherActor->Implements<UBallInterface>())
    {
        LastRimContactTime = GetWorld()->GetTimeSeconds();
    }
}

void AHoop::OnRimDetectorBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    const IBallInterface* Ball = Cast<IBallInterface>(OtherActor);
    if (!Ball)
    {
        return;
    }

    // Only track balls that are falling into the rim.
    if (Ball->GetBallVelocity().Z >= -MinimumDownwardVelocity)
    {
        return;
    }

    BallsAboveRim.Add(OtherActor);
    RimEntryTime = GetWorld()->GetTimeSeconds();

    const float CurrentHeight = OtherActor->GetActorLocation().Z;
    const float* PreviousMax = BallMaxHeights.Find(OtherActor);
    if (!PreviousMax || *PreviousMax < CurrentHeight)
    {
        BallMaxHeights.Add(OtherActor, CurrentHeight);
    }
}

void AHoop::OnScoreCylinderBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!OtherActor || !OtherActor->Implements<UBallInterface>() || !GameMode)
    {
        return;
    }

    // Only during a run, and only once per basket (ignore the ball bouncing around inside the net).
    if (GameMode->GetCurrentGameState() != EGameStateEnum::Playing || GameMode->IsBasketPending())
    {
        return;
    }

    if (!IsValidScoringAttempt(OtherActor))
    {
        return;
    }

    // Swish = no rim/backboard contact from shortly before entering the rim until now.
    bPendingSwish = LastRimContactTime < (RimEntryTime - SwishContactWindow);
    ScoringBall = OtherActor;

    BallsAboveRim.Remove(OtherActor);
    BallMaxHeights.Remove(OtherActor);

    // Freezes input and the round clock until the basket is awarded.
    GameMode->NotifyBasketPending();
    GetWorldTimerManager().SetTimer(SwitchSidesTimer, this, &AHoop::SwitchSides, SwitchSidesDelay, false);
}

bool AHoop::IsValidScoringAttempt(AActor* Ball) const
{
    const IBallInterface* BallInterface = Cast<IBallInterface>(Ball);
    if (!BallInterface)
    {
        return false;
    }

    if (!BallsAboveRim.Contains(Ball))
    {
        UE_LOG(LogFlappyHoop, Verbose, TEXT("Basket rejected: ball wasn't tracked above the rim"));
        return false;
    }

    const FVector BallVelocity = BallInterface->GetBallVelocity();
    if (BallVelocity.Z >= -MinimumDownwardVelocity)
    {
        UE_LOG(LogFlappyHoop, Verbose, TEXT("Basket rejected: ball not falling (Z=%f)"), BallVelocity.Z);
        return false;
    }

    const float* MaxHeight = BallMaxHeights.Find(Ball);
    if (!MaxHeight)
    {
        UE_LOG(LogFlappyHoop, Verbose, TEXT("Basket rejected: no max height recorded"));
        return false;
    }

    const float HeightDifference = *MaxHeight - ScoreCylinder->GetComponentLocation().Z;
    if (HeightDifference < MinimumHeightDifference)
    {
        UE_LOG(LogFlappyHoop, Verbose, TEXT("Basket rejected: height diff %f < %f"), HeightDifference, MinimumHeightDifference);
        return false;
    }

    return true;
}

void AHoop::ResetBallTracking()
{
    GetWorldTimerManager().ClearTimer(SwitchSidesTimer);
    BallsAboveRim.Empty();
    BallMaxHeights.Empty();
    bPendingSwish = false;
    LastRimContactTime = -1000.0;
    ScoringBall.Reset();
}

void AHoop::SwitchSides()
{
    // The run may have been left (Home) during the celebration delay.
    if (!GameMode || !GameMode->IsBasketPending())
    {
        return;
    }

    if (IBallInterface* BallInterface = Cast<IBallInterface>(ScoringBall.Get()))
    {
        BallInterface->ChangeBallDirection();
    }

    USoundBase* SoundToPlay = (bPendingSwish && SwishSound) ? SwishSound.Get() : ScoreSound.Get();
    UGameplayStatics::PlaySound2D(this, SoundToPlay);

    GameMode->ScoreBasket(bPendingSwish);

    bPendingSwish = false;
    ScoringBall.Reset();
}
