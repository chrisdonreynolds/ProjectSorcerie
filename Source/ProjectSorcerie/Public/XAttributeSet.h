// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "XAttributeSet.generated.h"

/**
 * 
 */
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class PROJECTSORCERIE_API UXAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
    // Constructor.
    UXAttributeSet();

    // The Gameplay Ability System needs to be able to replicate attribute values.
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    //Overide Attribute Changes to clamp their values
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;


    // --- Attributes ---

    // Health attribute, clamped between 0 and a MaxHealth attribute (which is not included in this example).
    UPROPERTY(BlueprintReadOnly, Category = "Attributes|Health", ReplicatedUsing = OnRep_Health)
    FGameplayAttributeData Health;
    ATTRIBUTE_ACCESSORS(UXAttributeSet, Health);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes|Health", ReplicatedUsing = OnRep_MaxHealth)
    FGameplayAttributeData MaxHealth;
    ATTRIBUTE_ACCESSORS(UXAttributeSet, MaxHealth);

    // Defense attribute, which might be used to mitigate incoming damage.
    UPROPERTY(BlueprintReadOnly, Category = "Attributes|Combat", ReplicatedUsing = OnRep_Defense)
    FGameplayAttributeData Defense;
    ATTRIBUTE_ACCESSORS(UXAttributeSet, Defense);

    // Accuracy attribute, used for determining the success rate of abilities.
    UPROPERTY(BlueprintReadOnly, Category = "Attributes|Combat", ReplicatedUsing = OnRep_Accuracy)
    FGameplayAttributeData Accuracy;
    ATTRIBUTE_ACCESSORS(UXAttributeSet, Accuracy);

    // Speed attribute, affecting character and ability movement speed.
    UPROPERTY(BlueprintReadOnly, Category = "Attributes|Movement", ReplicatedUsing = OnRep_Speed)
    FGameplayAttributeData Speed;
    ATTRIBUTE_ACCESSORS(UXAttributeSet, Speed);

    // Potency attribute, potentially increasing the effectiveness or damage of abilities.
    UPROPERTY(BlueprintReadOnly, Category = "Attributes|Sorcery", ReplicatedUsing = OnRep_Potency)
    FGameplayAttributeData Potency;
    ATTRIBUTE_ACCESSORS(UXAttributeSet, Potency);

protected:
    // --- Replication functions ---

    UFUNCTION()
    virtual void OnRep_Health(const FGameplayAttributeData& OldHealth);

    UFUNCTION()
    virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);

    UFUNCTION()
    virtual void OnRep_Defense(const FGameplayAttributeData& OldDefense);

    UFUNCTION()
    virtual void OnRep_Accuracy(const FGameplayAttributeData& OldAccuracy);

    UFUNCTION()
    virtual void OnRep_Speed(const FGameplayAttributeData& OldSpeed);

    UFUNCTION()
    virtual void OnRep_Potency(const FGameplayAttributeData& OldPotency);
	
};
