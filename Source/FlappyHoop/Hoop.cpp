// Fill out your copyright notice in the Description page of Project Settings.

#include "Hoop.h"
#include "BallInterface.h"
#include "GameModeInterface.h"
#include "FunctionsLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "MyPlayerController.h"
#include "Components/StaticMeshComponent.h"

// Sets default values
AHoop::AHoop()
{
    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
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

// Called when the game starts or when spawned
void AHoop::BeginPlay()
{
    Super::BeginPlay();

    World = GetWorld();

    ScoreCylinder->OnComponentBeginOverlap.AddDynamic(this, &AHoop::OnScoreCylinderBeginOverlap);
    RimDetector->OnComponentBeginOverlap.AddDynamic(this, &AHoop::OnRimDetectorBeginOverlap);

    GameModeInterface = UFunctionsLibrary::GetGameModeInterface(this);

    if (GameModeInterface)
    {
		GameModeInterface->OnGameResetDelegate().AddUObject(this, &AHoop::ResetBallTracking);
    }

    if (APlayerController* BasePlayerController = UGameplayStatics::GetPlayerController(this, 0))
        PlayerController = Cast<AMyPlayerController>(BasePlayerController);
}

// Called every frame
void AHoop::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AHoop::SetRingRotation(bool bRightEdgeHoop)
{
    FRotator InitialRotation = HoopRing->GetRelativeRotation();
    float PitchRotation = bRightEdgeHoop ? -10.0f : 10.0f;
    InitialRotation.Pitch = PitchRotation;
    HoopRing->SetRelativeRotation(InitialRotation);
}

void AHoop::OnRimDetectorBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (OtherActor && OtherActor->GetClass()->ImplementsInterface(UBallInterface::StaticClass()))
    {
        if (!BallInterface)
            BallInterface = Cast<IBallInterface>(OtherActor);

        if (BallInterface)
        {
            FVector BallVelocity = BallInterface->GetBallVelocity();

            //only track balls that are moving downward when entering the rim detector
            if (BallVelocity.Z < MinimumDownwardVelocity)
            {
                BallsAboveRim.Add(OtherActor);

                //record the maximum height this ball reached
                float CurrentHeight = OtherActor->GetActorLocation().Z;
                float ScoreHeight = ScoreCylinder->GetComponentLocation().Z;

                if (!BallMaxHeights.Contains(OtherActor) || BallMaxHeights[OtherActor] < CurrentHeight)
                {
                    BallMaxHeights.Add(OtherActor, CurrentHeight);
                }
            }
        }
    }
}

void AHoop::OnScoreCylinderBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (OtherActor && OtherActor->GetClass()->ImplementsInterface(UBallInterface::StaticClass()))
    {
        if (IsValidScoringAttempt(OtherActor))
        {
            if (PlayerController)
                PlayerController->DisableControllerInput();

            //clean up tracking for this ball
            BallsAboveRim.Remove(OtherActor);
            BallMaxHeights.Remove(OtherActor);

            World->GetTimerManager().SetTimer(GameModeInterface->GetSwitchSidesTimerHandle(), this, &AHoop::SwitchSides, SwitchSidesDelay, false);
        }

        else
        {
            GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("Invalid scoring attempt - ball did not come from above rim properly"));
        }
    }
}

bool AHoop::IsValidScoringAttempt(AActor* Ball)
{
    if (!Ball || !BallInterface || !Ball->GetClass()->ImplementsInterface(UBallInterface::StaticClass()))
        return false;

    //check if ball was tracked as being above the rim
    if (!BallsAboveRim.Contains(Ball))
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("Ball not tracked as being above rim"));
        return false;
    }

    FVector BallVelocity = BallInterface->GetBallVelocity();

    //ball must still be moving downward when scoring
    if (BallVelocity.Z >= -MinimumDownwardVelocity)
    {
        FString Message = FString::Printf(TEXT("Ball velocity too low: %f"), BallVelocity.Z);
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, Message);
        return false;
    }

    //check height difference, ball must have been significantly above the scoring cylinder
    float CurrentHeight = Ball->GetActorLocation().Z;
    float ScoreHeight = ScoreCylinder->GetComponentLocation().Z;

    if (BallMaxHeights.Contains(Ball))
    {
        float MaxHeight = BallMaxHeights[Ball];
        float HeightDifference = MaxHeight - ScoreHeight;

        if (HeightDifference < MinimumHeightDifference)
        {
            FString Message = FString::Printf(TEXT("Insufficient height difference: %f (required: %f)"), HeightDifference, MinimumHeightDifference);
            GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, Message);
            return false;
        }
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("No max height recorded for ball"));
        return false;
    }

    return true;
}

void AHoop::ResetBallTracking()
{
    BallsAboveRim.Empty();
    BallMaxHeights.Empty();
}

void AHoop::SwitchSides()
{
	EGameStateEnum CurrentGameState = GameModeInterface->GetCurrentGameState();

    if (CurrentGameState == EGameStateEnum::MainMenu)
		return;

    if (GameModeInterface)
    {
        BallInterface->ChangeBallDirection();
        PlayerController->EnableControllerInput();
        UGameplayStatics::PlaySound2D(this, ScoreSound);

        GameModeInterface->SetNewGameTime();
        GameModeInterface->UpdateScore();
        GameModeInterface->SetTimeEndedBool(false);
        GameModeInterface->OnPointScoredDelegate().Broadcast();
        GameModeInterface->UpdateScoreMultiplier();
        GameModeInterface->ActivateCoin();
    }
}