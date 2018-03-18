// Fill out your copyright notice in the Description page of Project Settings.

#include "CGCapturePoint.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Net/UnrealNetwork.h"

#include "CGPlayerCharacter.h"

ACGCapturePoint::ACGCapturePoint()
{
	PrimaryActorTick.bCanEverTick = true;

	OverlapComp = CreateDefaultSubobject<UBoxComponent>(TEXT("CapturePointOverlapComp"));
	RootComponent = OverlapComp;

	OverlapComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	OverlapComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	OverlapComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	
	OverlapComp->SetBoxExtent(FVector(200.0f));
	OverlapComp->SetupAttachment(MeshComp);

	OverlapComp->SetHiddenInGame(false);

	OverlapComp->OnComponentBeginOverlap.AddDynamic(this, &ACGCapturePoint::HandleBeginOverlap);
	OverlapComp->OnComponentEndOverlap.AddDynamic(this, &ACGCapturePoint::HandleEndOverlap);

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CapturePointMeshComp"));
	MeshComp->SetupAttachment(OverlapComp);

	/* TODO: Calculate progmatically based off of the number of overlapping players */
	CaptureRates.Add(5.0f);		// Takes 20 seconds for 1 Player to capture the point
	CaptureRates.Add(6.25f);	// Takes 16 Seconds for 2 Players
	CaptureRates.Add(8.33f);	// Takes 12 Seconds for 3 Players
	CaptureRates.Add(12.5f);	// Takes 8 Seconds for 4 Players
	
	CapturePointName = "DefaultName";

	DefendingTeam = "Neutral";
	CapturingTeam = "";
	AttackingTeam = "";

	CurrentCaptureRate = 0.0f;
	CurrentCapturePercentage = 0.0f;

	bReplicates = true;

	NetUpdateFrequency = 66.0f;
	MinNetUpdateFrequency = 33.0f;
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

float ACGCapturePoint::GetCurrentCapturePercentage()
{
	return CurrentCapturePercentage;
}

FName ACGCapturePoint::GetDefendingTeamName()
{
	if (DefendingTeam.IsEqual("Neutral"))
	{
		return CapturingTeam;
	}

	return DefendingTeam;
}

void ACGCapturePoint::ServerUpdateCaptureProgress_Implementation(float DeltaTime)
{
	float CaptureSpeed = 0.0f;

	uint8 NumAttackers = 0;
	uint8 NumDefenders = 0; 
	
	uint8 RedPlayersInCaptureRange = 0;
	uint8 BluePlayersInCaptureRange = 0;

	GetNumOverlappingPlayers(BluePlayersInCaptureRange, RedPlayersInCaptureRange);

	if (BluePlayersInCaptureRange == RedPlayersInCaptureRange)
	{
		return;
	}

	if (IsUncaptured())
	{
		CapturingTeam = BluePlayersInCaptureRange > RedPlayersInCaptureRange ? "Blue" : "Red";
	}
	
	NumAttackers = GetDefendingTeamName().IsEqual("Blue") ? RedPlayersInCaptureRange : BluePlayersInCaptureRange;
	NumDefenders = GetDefendingTeamName().IsEqual("Blue") ? BluePlayersInCaptureRange : RedPlayersInCaptureRange;

	if (CurrentCapturePercentage >= 100.0f && NumDefenders > NumAttackers)
	{
		return;
	}

	//Only allow a majority of +/- 4 players per team to influence the capture rate
	CaptureSpeed = FMath::Clamp((NumDefenders - NumAttackers), -4, 4);

	uint8 CaptureSpeedIdx = 0;

	if (CaptureSpeed < 0)
	{
		CaptureSpeedIdx = (CaptureSpeed * -1) - 1;

		CurrentCapturePercentage -= CaptureRates[CaptureSpeedIdx] * DeltaTime;
	}
	else if (CaptureSpeed > 0)
	{
		CaptureSpeedIdx = CaptureSpeed - 1;

		CurrentCapturePercentage += CaptureRates[CaptureSpeedIdx] * DeltaTime;
	}

	if (CurrentCapturePercentage <= 0.0f)
	{
		NotifyPointNeutralised(AttackingTeam);

		DefendingTeam = "Neutral";
		CapturingTeam = "";
	}
	else if (CurrentCapturePercentage >= 100.0f && !CapturingTeam.IsEqual(""))
	{
		NotifyPointCaptured(CapturingTeam);
		
		DefendingTeam = CapturingTeam;
		CapturingTeam = "";

		AttackingTeam = CapturingTeam.IsEqual("Blue") ? "Red" : "Blue";
	}
}

bool ACGCapturePoint::IsUncaptured()
{
	return DefendingTeam.IsEqual("Neutral") && CapturingTeam.IsEqual("");
}

bool ACGCapturePoint::ServerUpdateCaptureProgress_Validate(float DeltaTime)
{
	return true;
}

void ACGCapturePoint::GetNumOverlappingPlayers(uint8& OutNumBluePlayers, uint8& OutNumRedPlayers)
{
	TArray<AActor*> OverlappedActors;

	OverlapComp->GetOverlappingActors(OverlappedActors);

	if (OverlappedActors.Num() > 0)
	{
		for (AActor* OverlapActor : OverlappedActors)
		{
			ACGPlayerCharacter* Player = Cast<ACGPlayerCharacter>(OverlapActor);

			if (Player)
			{
				if (Player->GetTeamName().IsEqual("Blue"))
				{
					OutNumBluePlayers++;
				}
				else
				{
					OutNumRedPlayers++;
				}
			}
		}
	}
}

void ACGCapturePoint::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	ACGPlayerCharacter* Player = Cast<ACGPlayerCharacter>(OtherActor);

	if (Player)
	{
		Player->NotifyBeginOverlapCapturePoint(this);
	}
}

void ACGCapturePoint::HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ACGPlayerCharacter* Player = Cast<ACGPlayerCharacter>(OtherActor);

	if (Player)
	{
		Player->NotifyEndOverlapCapturePoint();
	}
}

void ACGCapturePoint::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACGCapturePoint, CurrentCapturePercentage);

	DOREPLIFETIME(ACGCapturePoint, DefendingTeam);
	DOREPLIFETIME(ACGCapturePoint, AttackingTeam);
	DOREPLIFETIME(ACGCapturePoint, CapturingTeam);
}