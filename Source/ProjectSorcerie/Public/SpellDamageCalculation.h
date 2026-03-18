// Spell Damage Execution Calculation
// This class calculates damage for all spells using attributes from UXAttributeSet.
// Spell-specific behavior is driven by gameplay tags read from the SpellData table.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "SpellDamageCalculation.generated.h"

class UDataTable;

/**
 * Calculates spell damage using Potency (attacker) and Defense (target).
 *
 * HOW IT WORKS:
 *   1. Reads the attacker's Potency and the target's Defense from UXAttributeSet.
 *   2. Reads the "Magnitude" from the GE spec's Level (set in the Spell blueprint from the data table).
 *   3. Gets the caster's SelectedSpell string, looks up that row in the SpellData table,
 *      and reads the SpellTags container directly.
 *   4. Applies a balanced damage formula with diminishing returns on defense.
 *   5. Checks SpellTags for spell-specific bonus logic (tag switches).
 *   6. Checks buff/debuff tags on source and target for global multipliers.
 *   7. Outputs the final damage as a negative Health modifier.
 *
 * DAMAGE FORMULA:
 *   BaseDamage      = Magnitude * (Potency * PotencyScalar)
 *   DefenseReduction = Defense / (Defense + DefenseConstant)       // 0 to 1, diminishing returns
 *   FinalDamage      = BaseDamage * (1 - DefenseReduction) * AllMultipliers
 *
 * HOW TO ADD A NEW SPELL'S CUSTOM LOGIC:
 *   1. Add a gameplay tag for the spell (e.g., "Spell.Fireball") to the spell data table row's SpellTags.
 *   2. In Execute_Implementation, add a new tag check block (see the "SPELL TAG SWITCHES" section).
 *   3. The tag check can read target/source tags and apply multipliers to Damage.
 */
UCLASS()
class PROJECTSORCERIE_API USpellDamageCalculation : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	USpellDamageCalculation();

	virtual void Execute_Implementation(
		const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;

protected:
	// Path to the SpellData data table asset.
	// Change this if your data table moves to a different location.
	FString SpellDataTablePath = TEXT("/Game/GameplayAbilities/SpellData/SpellData.SpellData");
};
