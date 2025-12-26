// Fill out your copyright notice in the Description page of Project Settings.


#include "XAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"

UXAttributeSet::UXAttributeSet()
    : Health(100.f), MaxHealth(100.f), Defense(100.f), Accuracy(100.f), Speed(100.f), Potency(100.f)
{

}

void UXAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UXAttributeSet, Health, OldHealth);
}

void UXAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UXAttributeSet, MaxHealth, OldMaxHealth);
}

void UXAttributeSet::OnRep_Defense(const FGameplayAttributeData& OldDefense)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UXAttributeSet, Defense, OldDefense);
}

void UXAttributeSet::OnRep_Accuracy(const FGameplayAttributeData& OldAccuracy)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UXAttributeSet, Accuracy, OldAccuracy);
}

void UXAttributeSet::OnRep_Speed(const FGameplayAttributeData& OldSpeed)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UXAttributeSet, Speed, OldSpeed);
}

void UXAttributeSet::OnRep_Potency(const FGameplayAttributeData& OldPotency)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UXAttributeSet, Potency, OldPotency);
}

void UXAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(UXAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UXAttributeSet, Health, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UXAttributeSet, Defense, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UXAttributeSet, Accuracy, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UXAttributeSet, Speed, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UXAttributeSet, Potency, COND_None, REPNOTIFY_Always);
}

void UXAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    // Always call the Super implementation
    Super::PreAttributeChange(Attribute, NewValue);

    // Health Clamping
    if (Attribute == GetHealthAttribute())
    {
        // Clamp the NewValue between 0 and the current MaxHealth value.
        NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
    }

    if (Attribute == GetDefenseAttribute())
    {
        // Clamp the NewValue between 0 and the current MaxHealth value.
        NewValue = FMath::Max(NewValue, 0.0f);
    }

    if (Attribute == GetSpeedAttribute())
    {
        // Clamp the NewValue between 0 and the current MaxHealth value.
        NewValue = FMath::Max(NewValue, 0.0f);
    }

    if (Attribute == GetAccuracyAttribute())
    {
        // Clamp the NewValue between 0 and the current MaxHealth value.
        NewValue = FMath::Max(NewValue, 0.0f);
    }

    if (Attribute == GetPotencyAttribute())
    {
        // Clamp the NewValue between 0 and the current MaxHealth value.
        NewValue = FMath::Max(NewValue, 0.0f);
    }
}
