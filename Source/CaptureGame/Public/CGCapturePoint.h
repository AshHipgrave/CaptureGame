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
	
	void GetNumOverlappingPlayers(uint8& OutNumBluePlayers, uint8& OutNumRedPlayers);

	bool IsUncaptured();

	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

protected:
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* MeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* OverlapComp;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Capture Behaviour")
	FName CapturePointName;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Capture Behaviour")
	TArray<float> CaptureRates;

	UPROPERTY(Replicated, VisibleDefaultsOnly, BlueprintReadOnly, Category = "Capture Behaviour")
	float CurrentCapturePercentage;

	float CurrentCaptureRate;

	UPROPERTY(Replicated)
	FName DefendingTeam;

	UPROPERTY(Replicated)
	FName AttackingTeam;

	UPROPERTY(Replicated)
	FName CapturingTeam;

public:	

	virtual void Tick(float DeltaTime) override;
	
	UFUNCTION(BlueprintCallable, Category = "Capture Behaviour")
	float GetCurrentCapturePercentage();

	UFUNCTION(BlueprintCallable, Category = "Capture Behaviour")
	FName GetDefendingTeamName();

	UFUNCTION(BlueprintImplementableEvent, Category = "Capture Behaviour")
	void NotifyPointNeutralised(FName NeutralisingTeam);

	UFUNCTION(BlueprintImplementableEvent, Category = "Capture Behaviour")
	void NotifyPointCaptured(FName CapuringTeam);
};
