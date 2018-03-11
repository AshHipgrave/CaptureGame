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

	//CaptureRates.Add(5.0f);	// Takes 20 seconds for 1 Player to capture the point
	//CaptureRates.Add(6.25f);	// Takes 16 Seconds for 2 Players
	//CaptureRates.Add(8.33f);	// Takes 12 Seconds for 3 Players
	//CaptureRates.Add(12.5f);	// Takes 8 Seconds for 4 Players
	
	DefendingTeam = "Neutral";
	AttackingTeam = "";
	CapturingTeam = "";

	CurrentCaptureRate = 0.0f;
	CurrentCapturePercentage = 0.0f;

	bReplicates = true;
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
	if (DefendingTeam.IsEqual(""))
	{
		return CapturingTeam;
	}

	return DefendingTeam;
}

void ACGCapturePoint::ServerUpdateCaptureProgress_Implementation(float DeltaTime)
{
	int32 CaptureSpeed = 0;

	uint32 BluePlayersInCaptureRange = 0;
	uint32 RedPlayersInCaptureRange = 0;

	GetNumOverlappingPlayers(BluePlayersInCaptureRange, RedPlayersInCaptureRange);

	uint32 TotalPlayersInCaptureRange = BluePlayersInCaptureRange + RedPlayersInCaptureRange;

	if (TotalPlayersInCaptureRange == 0)
	{
		return; //Nobody is overlapping, do nothing
	}

	// If we're neutral and nobody has started capturing the point then work out which team has the most players within 'Capture Range' and mark them as the capturing team
	if (DefendingTeam.IsEqual("Neutral") && CapturingTeam.IsEqual(""))
	{
		if (BluePlayersInCaptureRange > RedPlayersInCaptureRange)
		{
			CapturingTeam = "Blue";
			
			CaptureSpeed = (BluePlayersInCaptureRange - RedPlayersInCaptureRange);
		}
		else if (RedPlayersInCaptureRange > BluePlayersInCaptureRange)
		{
			CapturingTeam = "Red";

			CaptureSpeed = (RedPlayersInCaptureRange - BluePlayersInCaptureRange);
		}
		else
		{
			// There is no majority, do nothing
			return;
		}
	}
	else
	{
		// If we're being defended/captured from neutral, then work out the capture rate based off of the number of attackers vs. number of defenders

		uint32 NumAttackers = 0;
		uint32 NumDefenders = 0;

		if (DefendingTeam.IsEqual("Blue") || CapturingTeam.IsEqual("Blue"))
		{
			NumAttackers = RedPlayersInCaptureRange;
			NumDefenders = BluePlayersInCaptureRange;
		}
		else
		{
			NumAttackers = BluePlayersInCaptureRange;
			NumDefenders = RedPlayersInCaptureRange;
		}

		if (CurrentCapturePercentage >= 100.0f && NumDefenders > NumAttackers)
		{
			// The team with the majority already owns the point 100% - Don't do anything

			return;
		}
		else
		{
			CaptureSpeed = NumDefenders - NumAttackers;
		}
	}

	if (CaptureSpeed < 0)
	{
		// The point is being neutralised (The point is already owned and there are more attackers than defenders)
		CurrentCapturePercentage -= 5.0f * DeltaTime;
	}
	else if (CaptureSpeed > 0)
	{
		// The point is being captured from neutral
		CurrentCapturePercentage += 5.0f * DeltaTime;
	}

	FString CaptureProgressStr = FString::SanitizeFloat(CurrentCapturePercentage);
	UE_LOG(LogTemp, Log, TEXT("Capture Progress = %s"), *CaptureProgressStr);

	// Has the point been either neutralised or captured?
	if (CurrentCapturePercentage <= 0.0f)
	{
		CurrentCapturePercentage = 0.0f;

		if (!DefendingTeam.IsEqual("Neutral"))
		{
			FString LogStr = AttackingTeam.ToString();
			UE_LOG(LogTemp, Log, TEXT("%s team have neutralised a capture point!"), *LogStr);
		}

		// Reset the state back to Neutral
		DefendingTeam = "Neutral";
		AttackingTeam = "";
		CapturingTeam = "";
	}
	else if (CurrentCapturePercentage >= 100.0f)
	{
		CurrentCapturePercentage = 100.0f;

		// If true the point was captured from neutral (Either for the first time or from another team). If false then the defending team simply restored it back to 100% ownership without losing control
		if (!CapturingTeam.IsEqual(""))
		{
			FString LogStr = CapturingTeam.ToString();
			UE_LOG(LogTemp, Log, TEXT("%s team have captured a point!"), *LogStr);

			DefendingTeam = CapturingTeam;
			CapturingTeam = "";

			if (DefendingTeam.IsEqual("Blue"))
			{
				AttackingTeam = "Red";
			}
			else
			{
				AttackingTeam = "Blue";
			}
		}
	}
}

bool ACGCapturePoint::ServerUpdateCaptureProgress_Validate(float DeltaTime)
{
	return true;
}

void ACGCapturePoint::GetNumOverlappingPlayers(uint32& OutNumBluePlayers, uint32& OutNumRedPlayers)
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
		Player->NotifyEndOverlapCapturePoint(this);
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