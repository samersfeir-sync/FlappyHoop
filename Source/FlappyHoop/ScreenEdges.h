// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
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

private:

	UPROPERTY(EditAnywhere, Category = "Components")
	UStaticMeshComponent* EdgeMesh;

	UPROPERTY(EditInstanceOnly, Category = "Parameters")
	bool RightEdge = false;

	class IGameModeInterface* GameModeInterface = nullptr;

	void GetCameraFrustumEdges(class UCameraComponent* Camera, float Distance,
		FVector& LeftCenter, FVector& RightCenter);

	void SetEdgeLocation();
};
