// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CGProjectileWeaponBase.generated.h"

class UCameraShake;
class UParticleSystem;
class USkeletalMeshComponent;

USTRUCT()
struct FHitScanTrace
{
	/* Used to replicate the hit trace across the network for other players to visualise */

	GENERATED_BODY()

public:

	UPROPERTY()
	TEnumAsByte<EPhysicalSurface> SurfaceType;

	UPROPERTY()
	FVector_NetQuantize TraceTo;
};

UCLASS()
class CAPTUREGAME_API ACGProjectileWeaponBase : public AActor /* TODO: Make this more inline with how a 'base' class should be set out */
{
	GENERATED_BODY()
	
public:	

	ACGProjectileWeaponBase();

protected:

	/* Weapon functions */

	virtual void BeginPlay() override;

	void PlayWeaponFireEffects(FVector HitLocation);
	void PlayWeaponImpactEffects(EPhysicalSurface SurfaceType, FVector HitLocation);

	void Fire();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerFire();

	UFUNCTION()
	void OnRep_HitScanTrace();

protected:

	/* Weapon components and effects */

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* MeshComp;

	UPROPERTY(EditDefaultsOnly, BlueprintReadonly, Category = "Weapon")
	UParticleSystem* MuzzleFireEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	UParticleSystem* DefaultImpactEffect;

	// TODO: Setup a custom PhysicalSurface in editor 'Physics' settings(?-I think that's where it is) for flesh and then assign this to 'CGPlayerCharacter'
	//UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	//UParticleSystem* FleshImpactEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	UParticleSystem* TracerEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<UDamageType> DamageType;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<UCameraShake> FireCameraShake;

protected:

	/* Weapon behaviour parameters */

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName MuzzleSocketName;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName TracerTargetName; //This is hard coded into the particle effect as a required parameter

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float BaseWeaponDamage;

protected:

	/* Weapon fire control variables */
	
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float RateOfFire;

	float LastFireTime;

	float TimeBetweenShots;

	FTimerHandle TimerHandle_TimeBetweenShots;

	UPROPERTY(ReplicatedUsing=OnRep_HitScanTrace)
	FHitScanTrace HitScanTrace;

public:

	void StartFire();
	void StopFire();
};
