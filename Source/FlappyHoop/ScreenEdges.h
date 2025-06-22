// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/ArrowComponent.h"
#include "ScreenEdges.generated.h"

UCLASS()
class FLAPPYHOOP_API AScreenEdges : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AScreenEdges();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void ActivateEdge(bool bActivate);

private:

	UPROPERTY(EditAnywhere, Category = "Components")
	UStaticMeshComponent* EdgeMesh;

	UPROPERTY(EditInstanceOnly, Category = "Parameters")
	bool RightEdge = false;

	class IGameModeInterface* GameModeInterface = nullptr;

	void GameStarted();

	UPROPERTY(EditDefaultsOnly, Category = "Components")
	UChildActorComponent* HoopChild;

	UPROPERTY(EditDefaultsOnly, Category = "Components")
	UArrowComponent* UpArrow;

	UPROPERTY(EditDefaultsOnly, Category = "Components")
	UArrowComponent* MiddleArrow;

	UPROPERTY(EditDefaultsOnly, Category = "Components")
	UArrowComponent* DownArrow;

	TArray<UArrowComponent*> Arrows;

	void ResetEdge();

	UFUNCTION()
	void OnEdgeMeshBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY(EditInstanceOnly, Category = "Parameters")
	class ABall* Ball;
};
