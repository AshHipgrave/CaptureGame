// Fill out your copyright notice in the Description page of Project Settings.

#include "CGProjectileWeaponBase.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"

#include "CaptureGame.h"

ACGProjectileWeaponBase::ACGProjectileWeaponBase()
{
	MeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponSkeletalMeshComp"));
	RootComponent = MeshComp;

	RateOfFire = 600;
	BaseWeaponDamage = 20.0f;

	TracerTargetName = "BeamEnd";
	MuzzleSocketName = "MuzzleFlashSocket";

	SetReplicates(true);

	//Ensure the weapon replicates quickly (Weapon firing is too slow otherwise)
	NetUpdateFrequency = 66.0f;
	MinNetUpdateFrequency = 33.0f;
}


void ACGProjectileWeaponBase::BeginPlay()
{
	Super::BeginPlay();

	TimeBetweenShots = 60.0f / RateOfFire;	
}

void ACGProjectileWeaponBase::PlayWeaponFireEffects(FVector HitLocation)
{
	if (MuzzleFireEffect)
	{
		UGameplayStatics::SpawnEmitterAttached(MuzzleFireEffect, MeshComp, MuzzleSocketName);
	}

	if (TracerEffect)
	{
		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);

		UParticleSystemComponent* TracerComponent = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), TracerEffect, MuzzleLocation);

		if (TracerComponent)
		{
			TracerComponent->SetVectorParameter(TracerTargetName, HitLocation);
		}
	}

	APawn* WeaponOwner = Cast<APawn>(GetOwner());

	if (WeaponOwner)
	{
		APlayerController* PlayerController = Cast<APlayerController>(WeaponOwner->GetController());

		if (PlayerController)
		{
			PlayerController->ClientPlayCameraShake(FireCameraShake);
		}
	}
}

void ACGProjectileWeaponBase::PlayWeaponImpactEffects(EPhysicalSurface SurfaceType, FVector HitLocation)
{
	//TODO: Select a different effect based on the SurfaceType
	if (DefaultImpactEffect)
	{
		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);

		FVector ShotDirection = HitLocation - MuzzleLocation;
		ShotDirection.Normalize();

		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), DefaultImpactEffect, HitLocation, ShotDirection.Rotation());
	}
}

void ACGProjectileWeaponBase::Fire()
{
	if (Role < ROLE_Authority)
	{
		ServerFire();
	}

	AActor* WeaponOwner = GetOwner();

	if (WeaponOwner)
	{
		/* Perform a basic line trace */

		FVector EyeLocation;
		FRotator EyeRotation;

		FHitResult HitResult;

		WeaponOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);

		FVector ShotDirection = EyeRotation.Vector();
		FVector TraceEndLocation = EyeLocation + (ShotDirection * 10000 /* We have to end somewhere :D */);

		FCollisionQueryParams CollisionQueryParams;

		CollisionQueryParams.AddIgnoredActor(WeaponOwner);		// Ignore the weapon holder
		CollisionQueryParams.AddIgnoredActor(this);				// Ignore the weapon itself

		CollisionQueryParams.bTraceComplex = true;				// Perform a per triangle hit trace on the target rather than a simple collision so we know exactly where on the target we hit
		CollisionQueryParams.bReturnPhysicalMaterial = true;	// Store what type of PhysicalMaterial was hit (Not current implemented but future proofing)

		FVector TraceHitPoint = TraceEndLocation;

		EPhysicalSurface HitSurfaceType = SurfaceType_Default;

		if (GetWorld()->LineTraceSingleByChannel(HitResult, EyeLocation, TraceEndLocation, COLLISION_WEAPON, CollisionQueryParams))
		{
			AActor* HitActor = HitResult.GetActor();

			HitSurfaceType = UPhysicalMaterial::DetermineSurfaceType(HitResult.PhysMaterial.Get());

			//TODO: Apply extra damage for headshots!
			UGameplayStatics::ApplyPointDamage(HitActor, BaseWeaponDamage, ShotDirection, HitResult, WeaponOwner->GetInstigatorController(), this, DamageType);

			PlayWeaponImpactEffects(HitSurfaceType, HitResult.ImpactPoint);

			TraceHitPoint = HitResult.ImpactPoint;
		}

		PlayWeaponFireEffects(TraceHitPoint);

		if (Role == ROLE_Authority)
		{
			HitScanTrace.TraceTo = TraceHitPoint;
			HitScanTrace.SurfaceType = HitSurfaceType;
		}
		
		LastFireTime = GetWorld()->TimeSeconds;
	}
}

void ACGProjectileWeaponBase::OnRep_HitScanTrace()
{
	PlayWeaponFireEffects(HitScanTrace.TraceTo);

	PlayWeaponImpactEffects(HitScanTrace.SurfaceType, HitScanTrace.TraceTo);
}

void ACGProjectileWeaponBase::StartFire()
{
	float ShotDelay = FMath::Max(LastFireTime + TimeBetweenShots - GetWorld()->TimeSeconds, 0.0f);

	GetWorldTimerManager().SetTimer(TimerHandle_TimeBetweenShots, this, &ACGProjectileWeaponBase::Fire, TimeBetweenShots, true, ShotDelay);
}

void ACGProjectileWeaponBase::StopFire()
{
	GetWorldTimerManager().ClearTimer(TimerHandle_TimeBetweenShots);
}

void ACGProjectileWeaponBase::ServerFire_Implementation()
{
	Fire();
}

bool ACGProjectileWeaponBase::ServerFire_Validate()
{
	return true;
}

void ACGProjectileWeaponBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ACGProjectileWeaponBase, HitScanTrace, COND_SkipOwner); // Don't replicate the results of the hit back to the player who fired the shot (They already know the result!)
}