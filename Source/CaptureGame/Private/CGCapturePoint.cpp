// Fill out your copyright notice in the Description page of Project Settings.

#include "CGCapturePoint.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Net/UnrealNetwork.h"

#include "CGPlayerCharacter.h"

ACGCapturePoint::ACGCapturePoint()
{
	PrimaryActorTick.bCanEverTick = true;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CapturePointMeshComp"));
	RootComponent = MeshComp;

	OverlapComp = CreateDefaultSubobject<UBoxComponent>(TEXT("CapturePointOverlapComp"));

	OverlapComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	OverlapComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	OverlapComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	
	OverlapComp->SetBoxExtent(FVector(200.0f));
	OverlapComp->SetupAttachment(MeshComp);

	OverlapComp->SetHiddenInGame(false);
	OverlapComp->OnComponentBeginOverlap.AddDynamic(this, &ACGCapturePoint::HandleBeginOverlap);
	OverlapComp->OnComponentEndOverlap.AddDynamic(this, &ACGCapturePoint::HandleEndOverlap);

	//TODO: Calculate these based off of the number of overlapping players
	CaptureRates.Add(5.0f);
	CaptureRates.Add(6.25f);
	CaptureRates.Add(8.33f);
	CaptureRates.Add(12.5f);

	NumOverlappedAllies = 0;
	NumOverlappedEnemies = 0;

	DefendingTeam = "";

	CurrentCaptureRate = 0.0f;
	CurrentCapturePercentage = 0.0f;
}

void ACGCapturePoint::BeginPlay()
{
	Super::BeginPlay();
	
}

void ACGCapturePoint::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (Role == ROLE_Authority)
	{
		ServerUpdateCaptureProgress(DeltaTime);
	}
}

void ACGCapturePoint::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (Role != ROLE_Authority)
	{
		return;
	}

	ACGPlayerCharacter* Player = Cast<ACGPlayerCharacter>(OtherActor);

	if (Player && DefendingTeam.IsEqual(""))
	{
		DefendingTeam = Player->GetTeamName();
	}

	TArray<AActor*> OverlappedActors;

	OverlappedComponent->GetOverlappingActors(OverlappedActors);

	GetOverlappedCharacterCount(OverlappedActors);
}

void ACGCapturePoint::HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (Role != ROLE_Authority)
	{
		return;
	}

	TArray<AActor*> OverlappedActors;

	OverlappedComponent->GetOverlappingActors(OverlappedActors);

	GetOverlappedCharacterCount(OverlappedActors);
}

void ACGCapturePoint::GetOverlappedCharacterCount(TArray<AActor*>& OverlappedActors)
{
	NumOverlappedAllies = 0;
	NumOverlappedEnemies = 0;

	for (AActor* OverlapActor : OverlappedActors)
	{
		ACGPlayerCharacter* Player = Cast<ACGPlayerCharacter>(OverlapActor);

		if (Player)
		{
			if (Player->GetTeamName().IsEqual(DefendingTeam))
			{
				NumOverlappedAllies++;
			}
			else
			{
				NumOverlappedEnemies++;
			}
		}
	}

	FMath::Clamp(NumOverlappedAllies, 0, 12);
	FMath::Clamp(NumOverlappedEnemies, 0, 12);
}

void ACGCapturePoint::ServerUpdateCaptureProgress_Implementation(float DeltaTime)
{
	int32 CaptureSpeed = NumOverlappedAllies - NumOverlappedEnemies;

	if (CaptureSpeed == 0 || (CaptureSpeed > 0 && CurrentCapturePercentage >= 100.0f))
	{
		return;
	}

	FMath::Clamp(CaptureSpeed, -4, 4);

	float CaptureRate = CaptureRates[CaptureSpeed % CaptureRates.Num()];

	if (CaptureSpeed < 0)
	{
		CurrentCapturePercentage -= CaptureRate * DeltaTime;
	}
	else
	{
		CurrentCapturePercentage += CaptureRate * DeltaTime;
	}

	FMath::Clamp(CurrentCapturePercentage, 0.0f, 100.0f);

	// Point has been captured, switch the team
	if (CurrentCapturePercentage == 0.0f)
	{
		if (DefendingTeam.IsEqual("Blue"))
		{
			DefendingTeam = "Red";
		}
		else
		{
			DefendingTeam = "Blue";
		}
	}

	FString CaptureProgressStr = FString::SanitizeFloat(CurrentCapturePercentage);
	UE_LOG(LogTemp, Log, TEXT("Capture Progress = %s"), *CaptureProgressStr);
}

bool ACGCapturePoint::ServerUpdateCaptureProgress_Validate(float DeltaTime)
{
	return true;
}

void ACGCapturePoint::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACGCapturePoint, CurrentCapturePercentage);
	DOREPLIFETIME(ACGCapturePoint, DefendingTeam);
}