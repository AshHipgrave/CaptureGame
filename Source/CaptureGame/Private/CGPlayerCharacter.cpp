// Fill out your copyright notice in the Description page of Project Settings.

#include "CGPlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Net/UnrealNetwork.h"

#include "CGProjectileWeaponBase.h"

ACGPlayerCharacter::ACGPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("PlayerSpringArmComponent"));
	SpringArmComp->bUsePawnControlRotation = true;
	SpringArmComp->SetupAttachment(RootComponent);

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("PlayerCameraComponent"));
	CameraComp->SetupAttachment(SpringArmComp);

	GetMovementComponent()->GetNavAgentPropertiesRef().bCanCrouch = true;

	WeaponSocketName = "WeaponSocket";

	CurrentTeam = "Blue";
}

// Called when the game starts or when spawned
void ACGPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	if (Role == ROLE_Authority)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn; // Always spawn objects even if they're colliding with something (Otherwise the weapon wouldn't spawn because it's constantly colliding with the hand)

		CurrentWeapon = GetWorld()->SpawnActor<ACGProjectileWeaponBase>(DefaultStartWeapon, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

		if (CurrentWeapon)
		{
			CurrentWeapon->SetOwner(this);
			CurrentWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponSocketName);
		}
	}
}

void ACGPlayerCharacter::MoveForward(float Value)
{
	AddMovementInput(GetActorForwardVector() * Value);
}

void ACGPlayerCharacter::MoveRight(float Value)
{
	AddMovementInput(GetActorRightVector() * Value);
}

void ACGPlayerCharacter::BeginCrouch()
{
	Crouch();
}

void ACGPlayerCharacter::EndCrouch()
{
	UnCrouch();
}

void ACGPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ACGPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// TODO: Add Aim-Down-Sight support.

	PlayerInputComponent->BindAxis("MoveForward", this, &ACGPlayerCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ACGPlayerCharacter::MoveRight);

	PlayerInputComponent->BindAxis("LookUp", this, &ACGPlayerCharacter::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("Turn", this, &ACGPlayerCharacter::AddControllerYawInput);

	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ACGPlayerCharacter::BeginCrouch);
	PlayerInputComponent->BindAction("Crouch", IE_Released, this, &ACGPlayerCharacter::EndCrouch);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACGPlayerCharacter::Jump);

	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ACGPlayerCharacter::StartWeaponFire);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ACGPlayerCharacter::StopWeaponFire);
}

FVector ACGPlayerCharacter::GetPawnViewLocation() const
{
	if (CameraComp)
	{
		return CameraComp->GetComponentLocation();
	}

	return Super::GetPawnViewLocation();
}

void ACGPlayerCharacter::StartWeaponFire()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StartFire();
	}
}

void ACGPlayerCharacter::StopWeaponFire()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StopFire();
	}
}

FName ACGPlayerCharacter::GetTeamName()
{
	return CurrentTeam;
}

void ACGPlayerCharacter::SwitchTeam()
{
	/* Temp function, callable from the console to easily test team switching */

	if (CurrentTeam.IsEqual("Blue"))
	{
		CurrentTeam = "Red";
	}
	else
	{
		CurrentTeam = "Blue";
	}
}

void ACGPlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACGPlayerCharacter, bIsDead);
	DOREPLIFETIME(ACGPlayerCharacter, CurrentWeapon);
}
