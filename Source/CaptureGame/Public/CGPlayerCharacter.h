// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CGPlayerCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class ACGProjectileWeaponBase;

UCLASS()
class CAPTUREGAME_API ACGPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:

	ACGPlayerCharacter();

protected:

	virtual void BeginPlay() override;

	void MoveForward(float Value);
	void MoveRight(float Value);

	void BeginCrouch();
	void EndCrouch();

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCameraComponent* CameraComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USpringArmComponent* SpringArmComp;

	UPROPERTY(Replicated)
	ACGProjectileWeaponBase* CurrentWeapon;

	UPROPERTY(EditDefaultsOnly, Category = "Player")
	TSubclassOf<ACGProjectileWeaponBase> DefaultStartWeapon;

	UPROPERTY(VisibleDefaultsOnly, Category = "Player")
	FName WeaponSocketName;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player")
	bool bIsDead;

public:	

	virtual void Tick(float DeltaTime) override;
	
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual FVector GetPawnViewLocation() const override;	

	UFUNCTION(BlueprintCallable, Category = "Player")
	void StartWeaponFire();

	UFUNCTION(BlueprintCallable, Category = "Player")
	void StopWeaponFire();
};
