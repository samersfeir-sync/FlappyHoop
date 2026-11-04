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
    // Ticks for the drift (Hard) and to re-check balls sitting inside the rim (see Tick).
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;

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
    ScoreCylinder->OnComponentEndOverlap.AddUniqueDynamic(this, &AHoop::OnScoreCylinderEndOverlap);
    RimDetector->OnComponentBeginOverlap.AddUniqueDynamic(this, &AHoop::OnRimDetectorBeginOverlap);
    RimDetector->OnComponentEndOverlap.AddUniqueDynamic(this, &AHoop::OnRimDetectorEndOverlap);

    // Blocking hits from the ball's projectile sweep are dispatched to these components too (same as Ground's OnActorHit).
    HoopMesh->OnComponentHit.AddUniqueDynamic(this, &AHoop::OnHoopPartHit);
    HoopRing->OnComponentHit.AddUniqueDynamic(this, &AHoop::OnHoopPartHit);

    GameMode = ABaseGameMode::Get(this);
    if (GameMode)
    {
        GameMode->OnGameReset.AddUObject(this, &AHoop::ResetBallTracking);
    }
}

void AHoop::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (GameMode)
    {
        GameMode->OnGameReset.RemoveAll(this);
    }

    Super::EndPlay(EndPlayReason);
}

void AHoop::StartDrift(double BaseZ, double MinZ, double MaxZ)
{
    DriftBaseZ = BaseZ;
    DriftMinZ = FMath::Min(MinZ, MaxZ);
    DriftMaxZ = FMath::Max(MinZ, MaxZ);
    DriftPhase = 0.0;
    DriftSign = FMath::RandBool() ? 1.0 : -1.0;

    // Amount and speed are read every tick, so a level change mid-placement is picked up too.
    bDriftActive = GameMode != nullptr;
}

void AHoop::StopDrift()
{
    bDriftActive = false;
    HoopVelocityZ = 0.0;
}

void AHoop::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    HoopVelocityZ = 0.0;

    // Hold still during the basket celebration, the second-chance panel and outside a run.
    if (!GameMode || GameMode->GetCurrentGameState() != EGameStateEnum::Playing || GameMode->IsBasketPending())
    {
        return;
    }

    const double Amplitude = GameMode->GetHoopDriftAmplitude();
    if (bDriftActive && Amplitude > 0.0 && DeltaSeconds > 0.0f)
    {
        DriftPhase += DeltaSeconds * GameMode->GetHoopDriftCyclesPerSecond();

        FVector Location = GetActorLocation();
        const double OldZ = Location.Z;
        Location.Z = FMath::Clamp(DriftBaseZ + DriftSign * Amplitude * FMath::Sin(DriftPhase * UE_DOUBLE_TWO_PI), DriftMinZ, DriftMaxZ);
        SetActorLocation(Location);
        HoopVelocityZ = (Location.Z - OldZ) / DeltaSeconds;
    }

    // Drop marks of balls that left the rim without scoring (missed, fell out the side) expire.
    const double Now = GetWorld()->GetTimeSeconds();
    for (auto It = RimEntries.CreateIterator(); It; ++It)
    {
        if (!It->Key.IsValid() || (!BallsInRim.Contains(It->Key) && Now - It->Value.Time > RimToScoreMaxTime))
        {
            It.RemoveCurrent();
        }
    }

    // Overlap events alone miss some real baskets: a ball that enters the rim near the top of its arc (or with the
    // hoop drifting down onto it) is still rising on entry, then falls through without a new "enter rim" event;
    // and when the hoop moves, both zones can start overlapping in the same frame in the wrong order.
    // So re-check balls sitting in the rim every frame.
    for (auto It = BallsInRim.CreateIterator(); It; ++It)
    {
        AActor* Ball = It->Get();
        if (!Ball)
        {
            It.RemoveCurrent();
            continue;
        }

        TryMarkRimEntry(Ball);

        if (FindFreshRimEntry(Ball) && ScoreCylinder->IsOverlappingActor(Ball))
        {
            TryScore(Ball, false);
            if (GameMode->IsBasketPending())
            {
                break;
            }
        }
    }
}

double AHoop::GetRelativeVelocityZ(const AActor* Ball) const
{
    const IBallInterface* BallInterface = Cast<IBallInterface>(Ball);
    return BallInterface ? BallInterface->GetBallVelocity().Z - HoopVelocityZ : 0.0;
}

void AHoop::TryMarkRimEntry(AActor* Ball)
{
    if (!Ball || CameFromBelow.Contains(Ball))
    {
        return;
    }

    // Falling relative to the hoop, with its centre above the score zone: it's coming down through the rim.
    if (GetRelativeVelocityZ(Ball) >= -MinimumDownwardVelocity)
    {
        return;
    }

    const double HeightAboveZone = Ball->GetActorLocation().Z - ScoreCylinder->Bounds.Origin.Z;
    if (HeightAboveZone <= 0.0)
    {
        return;
    }

    const double Now = GetWorld()->GetTimeSeconds();
    FRimEntry* Existing = RimEntries.Find(Ball);
    if (Existing && Now - Existing->Time <= RimToScoreMaxTime)
    {
        // Still dropping inside the rim (e.g. rattling): same attempt, keep it fresh.
        Existing->Time = Now;
        Existing->HeightAboveZone = HeightAboveZone;
        return;
    }

    // New drop (or an old, expired one): start over.
    FRimEntry& Entry = Existing ? *Existing : RimEntries.Add(Ball);
    Entry.Time = Now;
    Entry.FirstTime = Now;
    Entry.HeightAboveZone = HeightAboveZone;
}

const AHoop::FRimEntry* AHoop::FindFreshRimEntry(const AActor* Ball) const
{
    const FRimEntry* Entry = RimEntries.Find(Ball);
    return (Entry && GetWorld()->GetTimeSeconds() - Entry->Time <= RimToScoreMaxTime) ? Entry : nullptr;
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
    if (!OtherActor || !OtherActor->Implements<UBallInterface>())
    {
        return;
    }

    BallsInRim.Add(OtherActor);
    TryMarkRimEntry(OtherActor);
}

void AHoop::OnRimDetectorEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (!OtherActor || !OtherActor->Implements<UBallInterface>())
    {
        return;
    }

    BallsInRim.Remove(OtherActor);

    // Left the rim area going up (bounced out / flapped away): this drop is over.
    if (GetRelativeVelocityZ(OtherActor) > 0.0)
    {
        RimEntries.Remove(OtherActor);
    }

    if (!ScoreCylinder->IsOverlappingActor(OtherActor))
    {
        CameFromBelow.Remove(OtherActor);
    }
}

void AHoop::OnScoreCylinderEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (OtherActor && !BallsInRim.Contains(OtherActor))
    {
        CameFromBelow.Remove(OtherActor);
    }
}

void AHoop::OnScoreCylinderBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!OtherActor || !OtherActor->Implements<UBallInterface>() || !GameMode)
    {
        return;
    }

    // Both zones can start overlapping in the same frame (fast ball / moving hoop): give the rim the first look.
    if (BallsInRim.Contains(OtherActor))
    {
        TryMarkRimEntry(OtherActor);
    }

    TryScore(OtherActor, true);
}

void AHoop::TryScore(AActor* OtherActor, bool bFromOverlapEvent)
{
    // Only during a run, and only once per basket (ignore the ball bouncing around inside the net).
    if (!GameMode || GameMode->GetCurrentGameState() != EGameStateEnum::Playing || GameMode->IsBasketPending())
    {
        return;
    }

    FString Reason;
    if (!IsValidScoringAttempt(OtherActor, Reason))
    {
        if (!bFromOverlapEvent)
        {
            return;
        }

        const double RelativeVelocityZ = GetRelativeVelocityZ(OtherActor);

        // Rising with no drop recorded = it came from below: void until it's clear of the hoop.
        // Rising WITH a drop recorded = it bounced off the rim on its way down; keep the drop, it scores once it falls again.
        // Falling with no drop recorded yet = both zones overlapped in the same frame; Tick marks and scores it.
        const bool bFromBelow = RelativeVelocityZ >= 0.0 && !FindFreshRimEntry(OtherActor);
        if (bFromBelow)
        {
            RimEntries.Remove(OtherActor);
            CameFromBelow.Add(OtherActor);
        }

        if (bLogScoringDecisions)
        {
            UE_LOG(LogFlappyHoop, Log, TEXT("Basket %s (%s): %s (ball vZ %.0f, hoop vZ %.0f)"),
                bFromBelow ? TEXT("rejected, came from below") : TEXT("not yet, still watching"), *GetName(), *Reason,
                RelativeVelocityZ + HoopVelocityZ, HoopVelocityZ);
        }
        return;
    }

    // Swish = no rim/backboard contact from shortly before entering the rim until now.
    const double RimEntryTime = RimEntries.FindChecked(OtherActor).FirstTime;
    bPendingSwish = LastRimContactTime < (RimEntryTime - SwishContactWindow);
    ScoringBall = OtherActor;

    if (bLogScoringDecisions)
    {
        UE_LOG(LogFlappyHoop, Log, TEXT("Basket accepted (%s), swish=%d"), *GetName(), bPendingSwish ? 1 : 0);
    }

    RimEntries.Remove(OtherActor);

    // Freezes input and the round clock until the basket is awarded.
    GameMode->NotifyBasketPending();
    GetWorldTimerManager().SetTimer(SwitchSidesTimer, this, &AHoop::SwitchSides, SwitchSidesDelay, false);
}

bool AHoop::IsValidScoringAttempt(AActor* Ball, FString& OutReason) const
{
    const IBallInterface* BallInterface = Cast<IBallInterface>(Ball);
    if (!BallInterface)
    {
        OutReason = TEXT("not a ball");
        return false;
    }

    const FRimEntry* Entry = RimEntries.Find(Ball);
    if (!Entry)
    {
        OutReason = TEXT("didn't drop into the rim first");
        return false;
    }

    const double SinceRim = GetWorld()->GetTimeSeconds() - Entry->Time;
    if (SinceRim > RimToScoreMaxTime)
    {
        OutReason = FString::Printf(TEXT("rim entry too old (%.2fs > %.2fs), not one drop"), SinceRim, RimToScoreMaxTime);
        return false;
    }

    const double VelocityZ = GetRelativeVelocityZ(Ball);
    if (VelocityZ >= -MinimumDownwardVelocity)
    {
        OutReason = FString::Printf(TEXT("not falling relative to the hoop (Z velocity %.0f)"), VelocityZ);
        return false;
    }

    // It must have come into the rim from above the cylinder, not from beside/under it.
    if (Entry->HeightAboveZone <= 0.0)
    {
        OutReason = FString::Printf(TEXT("entered the rim below the score zone (%.0f above zone centre)"), Entry->HeightAboveZone);
        return false;
    }

    return true;
}

void AHoop::ResetBallTracking()
{
    GetWorldTimerManager().ClearTimer(SwitchSidesTimer);
    RimEntries.Empty();
    BallsInRim.Empty();
    CameFromBelow.Empty();
    HoopVelocityZ = 0.0;
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

    USoundBase* SoundToPlay = (bPendingSwish && SwishSound) ? SwishSound.Get() : ScoreSound.Get();
    UGameplayStatics::PlaySound2D(this, SoundToPlay);

    // Score first: it can raise the level, and the next hoop is placed (height band, drift) with the new level.
    GameMode->ScoreBasket(bPendingSwish);

    if (IBallInterface* BallInterface = Cast<IBallInterface>(ScoringBall.Get()))
    {
        BallInterface->ChangeBallDirection();
    }

    bPendingSwish = false;
    ScoringBall.Reset();
}
