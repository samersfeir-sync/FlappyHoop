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

    FRimEntry& Entry = RimEntries.FindOrAdd(OtherActor);
    Entry.Time = GetWorld()->GetTimeSeconds();
    Entry.BallZ = OtherActor->GetActorLocation().Z;
}

void AHoop::OnRimDetectorEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    const IBallInterface* Ball = Cast<IBallInterface>(OtherActor);
    if (!Ball)
    {
        return;
    }

    // Left the rim area going up (bounced out / flapped away): this drop is over.
    if (Ball->GetBallVelocity().Z > 0.0f)
    {
        RimEntries.Remove(OtherActor);
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

    FString Reason;
    if (!IsValidScoringAttempt(OtherActor, Reason))
    {
        if (bLogScoringDecisions)
        {
            UE_LOG(LogFlappyHoop, Log, TEXT("Basket rejected (%s): %s"), *GetName(), *Reason);
        }

        // Touching the cylinder while rising means it came from below: whatever drop it had is void.
        if (const IBallInterface* Ball = Cast<IBallInterface>(OtherActor); Ball && Ball->GetBallVelocity().Z >= 0.0f)
        {
            RimEntries.Remove(OtherActor);
        }
        return;
    }

    // Swish = no rim/backboard contact from shortly before entering the rim until now.
    const double RimEntryTime = RimEntries.FindChecked(OtherActor).Time;
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

    const float VelocityZ = BallInterface->GetBallVelocity().Z;
    if (VelocityZ >= -MinimumDownwardVelocity)
    {
        OutReason = FString::Printf(TEXT("not falling (Z velocity %.0f)"), VelocityZ);
        return false;
    }

    // It must have come into the rim from above the cylinder, not from beside/under it.
    const double CylinderCenterZ = ScoreCylinder->Bounds.Origin.Z;
    if (Entry->BallZ <= CylinderCenterZ)
    {
        OutReason = FString::Printf(TEXT("entered the rim below the score zone (ball Z %.0f, zone centre %.0f)"), Entry->BallZ, CylinderCenterZ);
        return false;
    }

    return true;
}

void AHoop::ResetBallTracking()
{
    GetWorldTimerManager().ClearTimer(SwitchSidesTimer);
    RimEntries.Empty();
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
