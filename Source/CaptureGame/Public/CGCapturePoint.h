// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CGCapturePoint.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

UCLASS()
class CAPTUREGAME_API ACGCapturePoint : public AActor
{
	GENERATED_BODY()
	
public:	

	ACGCapturePoint();

protected:

	virtual void BeginPlay() override;
	
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerUpdateCaptureProgress(float DeltaTime);
	
	void GetNumOverlappingPlayers(uint32& OutNumBluePlayers, uint32& OutNumRedPlayers);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* MeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* OverlapComp;
	
protected:
	
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Capture Behaviour")
	TArray<float> CaptureRates;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Capture Behaviour")
	float CurrentCapturePercentage;

	float CurrentCaptureRate;

	UPROPERTY(Replicated)
	FName DefendingTeam;

	UPROPERTY(Replicated)
	FName CapturingTeam;

	UPROPERTY(Replicated)
	FName AttackingTeam;

public:	

	virtual void Tick(float DeltaTime) override;
	
	UFUNCTION(BlueprintCallable, Category = "Capture Behaviour")
	float GetCurrentCapturePercentage();
};
