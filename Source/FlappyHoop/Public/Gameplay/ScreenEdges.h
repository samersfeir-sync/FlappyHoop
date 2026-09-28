// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ScreenEdges.generated.h"

class UStaticMeshComponent;
class UChildActorComponent;
class UArrowComponent;
class ABaseGameMode;
class ABall;
class AHoop;

UCLASS()
class FLAPPYHOOP_API AScreenEdges : public AActor
{
	GENERATED_BODY()

public:

	AScreenEdges();

	void ActivateEdge(bool bActivate);

protected:

	virtual void BeginPlay() override;

private:

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> EdgeMesh;

	UPROPERTY(EditInstanceOnly, Category = "Parameters")
	bool bRightEdge = false;

	UPROPERTY(Transient)
	TObjectPtr<ABaseGameMode> GameMode = nullptr;

	void GameStarted() { ActivateEdge(!bRightEdge); }

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UChildActorComponent> HoopChild;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UArrowComponent> UpArrow;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UArrowComponent> MiddleArrow;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UArrowComponent> DownArrow;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UArrowComponent>> Arrows;

	void ResetEdge();

	UFUNCTION()
	void OnEdgeMeshBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY(EditInstanceOnly, Category = "Parameters")
	TObjectPtr<ABall> Ball;
};
