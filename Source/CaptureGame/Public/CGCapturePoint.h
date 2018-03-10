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

	void GetOverlappedCharacterCount(TArray<AActor*>& OverlappedActors);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* MeshComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* OverlapComp;
	
protected:
	
	UPROPERTY(EditInstanceOnly, Category = "Behaviour")
	TArray<float> CaptureRates;

	UPROPERTY(Replicated)
	float CurrentCapturePercentage;

	float CurrentCaptureRate;

	UPROPERTY(Replicated)
	FName DefendingTeam;

	int32 NumOverlappedAllies;
	int32 NumOverlappedEnemies;

public:	

	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	
};
