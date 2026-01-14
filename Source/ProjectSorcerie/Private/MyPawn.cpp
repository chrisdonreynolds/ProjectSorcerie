// Fill out your copyright notice in the Description page of Project Settings.


#include "MyPawn.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "XAttributeSet.h"

// Sets default values
AMyPawn::AMyPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UXAttributeSet>(TEXT("AttributeSet"));

	SelectedSpell = TEXT(" ");

}

UAbilitySystemComponent* AMyPawn::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}


// Called when the game starts or when spawned
void AMyPawn::BeginPlay()
{
	Super::BeginPlay();
	
	InitializeAttributes();
}


void AMyPawn::InitializeAttributes()
{
	if (AbilitySystemComponent && AttributeSet)
	{


	}

}



// Called every frame
void AMyPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AMyPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

