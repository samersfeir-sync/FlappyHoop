// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Hoop.generated.h"

UCLASS()
class FLAPPYHOOP_API AHoop : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AHoop();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:

	UPROPERTY(EditAnywhere, Category = "Components")
	UStaticMeshComponent* HoopMesh;

	UPROPERTY(EditAnywhere, Category = "Components")
	UStaticMeshComponent* ScoreCylinder;
	
	UFUNCTION()
	void OnScoreCylinderBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	class IGameModeInterface* GameModeInterface = nullptr;
};
