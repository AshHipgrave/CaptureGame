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
	
	CapturePointName = "DefaultName";

	DefendingTeam = "Neutral";
	CapturingTeam = "";
	AttackingTeam = "";

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
	int NumAttackers = 0;
	int NumDefenders = 0;

	int RedPlayersInCaptureRange = 0;
	int BluePlayersInCaptureRange = 0;

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

	if (CurrentCapturePercentage >= 100.0f && NumDefenders >= NumAttackers)
	{
		return;
	}

	//Only allow a majority of 4 players per team to influence the capture rate
	NumAttackers = FMath::Clamp(NumAttackers, 0, 4);
	NumDefenders = FMath::Clamp(NumDefenders, 0, 4);

	CurrentCapturePercentage += CalculateCaptureRate(NumDefenders, NumAttackers) * DeltaTime;

	FString CaptureProgressStr = FString::SanitizeFloat(CurrentCapturePercentage);

	UE_LOG(LogTemp, Log, TEXT("Capture Progress = %s"), *CaptureProgressStr);

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

		AttackingTeam = GetDefendingTeamName().IsEqual("Blue") ? "Red" : "Blue";
	}
}

int ACGCapturePoint::CalculateCaptureRate(int numDefenders, int numAttackers)
{
	if (numDefenders > numAttackers)
	{
		return -4 * ((numDefenders - numAttackers) - 6);
	}
	
	return 4 * ((numAttackers - numDefenders) - 6);
}

bool ACGCapturePoint::IsUncaptured()
{
	return DefendingTeam.IsEqual("Neutral") && CapturingTeam.IsEqual("");
}

bool ACGCapturePoint::ServerUpdateCaptureProgress_Validate(float DeltaTime)
{
	return true;
}

void ACGCapturePoint::GetNumOverlappingPlayers(int& OutNumBluePlayers, int& OutNumRedPlayers)
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