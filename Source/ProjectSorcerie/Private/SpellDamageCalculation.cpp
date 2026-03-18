// Spell Damage Execution Calculation

#include "SpellDamageCalculation.h"
#include "XAttributeSet.h"
#include "MyPawn.h"
#include "AbilitySystemComponent.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"

// ============================================================================
// ATTRIBUTE CAPTURE SETUP
// This tells the Gameplay Effect system which attributes we need to read.
// ============================================================================

struct FSpellDamageCapture
{
	// These macros declare the capture variables for each attribute.
	DECLARE_ATTRIBUTE_CAPTUREDEF(Potency);
	DECLARE_ATTRIBUTE_CAPTUREDEF(Defense);
	DECLARE_ATTRIBUTE_CAPTUREDEF(Health);

	FSpellDamageCapture()
	{
		// Source = the caster, Target = the enemy being hit.
		// The last parameter (false) means "don't snapshot" — use the live value at time of application.
		DEFINE_ATTRIBUTE_CAPTUREDEF(UXAttributeSet, Potency, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UXAttributeSet, Defense, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UXAttributeSet, Health, Target, false);
	}
};

// Singleton accessor so we only create the capture struct once.
static const FSpellDamageCapture& GetSpellDamageCapture()
{
	static FSpellDamageCapture Capture;
	return Capture;
}

// ============================================================================
// HELPER: Look up a spell row in the data table and extract the SpellTags.
// The SpellData table uses a Blueprint-defined struct, so we use generic
// FProperty access to read the SpellTags field by name.
// ============================================================================

static FGameplayTagContainer GetSpellTagsFromDataTable(const UDataTable* DataTable, const FString& SpellCode)
{
	FGameplayTagContainer OutTags;

	if (!DataTable || SpellCode.IsEmpty())
	{
		return OutTags;
	}

	// Find the row by name. Returns a raw pointer to the row data.
	const FName RowName(*SpellCode);
	const uint8* RowData = DataTable->FindRowUnchecked(RowName);
	if (!RowData)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpellDamageCalc: Could not find row '%s' in SpellData table."), *SpellCode);
		return OutTags;
	}

	// Get the struct that defines the rows in this table.
	const UScriptStruct* RowStruct = DataTable->GetRowStruct();
	if (!RowStruct)
	{
		return OutTags;
	}

	// Find the "SpellTags" property by name.
	const FProperty* TagsProp = RowStruct->FindPropertyByName(FName("SpellTags"));
	if (!TagsProp)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpellDamageCalc: 'SpellTags' property not found in row struct."));
		return OutTags;
	}

	// Get a pointer to the actual FGameplayTagContainer inside the row.
	const FStructProperty* StructProp = CastField<FStructProperty>(TagsProp);
	if (StructProp && StructProp->Struct == FGameplayTagContainer::StaticStruct())
	{
		const FGameplayTagContainer* TagContainer =
			StructProp->ContainerPtrToValuePtr<FGameplayTagContainer>(RowData);
		if (TagContainer)
		{
			OutTags = *TagContainer;
		}
	}

	return OutTags;
}

// ============================================================================
// CONSTRUCTOR — Register which attributes this calculation needs to capture.
// ============================================================================

USpellDamageCalculation::USpellDamageCalculation()
{
	RelevantAttributesToCapture.Add(GetSpellDamageCapture().PotencyDef);
	RelevantAttributesToCapture.Add(GetSpellDamageCapture().DefenseDef);
	RelevantAttributesToCapture.Add(GetSpellDamageCapture().HealthDef);
}

// ============================================================================
// EXECUTE — The actual damage calculation. This runs when the GE is applied.
// ============================================================================

void USpellDamageCalculation::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	// ------------------------------------------------------------------
	// STEP 1: Get the gameplay effect spec and tag containers.
	// ------------------------------------------------------------------
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	// Source tags = tags on the caster (buffs, states, etc.)
	// Target tags = tags on the target (debuffs, states, etc.)
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	// This struct is needed by the attribute capture system.
	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = SourceTags;
	EvalParams.TargetTags = TargetTags;

	// ------------------------------------------------------------------
	// STEP 2: Read the captured attribute values.
	// ------------------------------------------------------------------

	// Potency = how strong the caster's spells are.
	float Potency = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		GetSpellDamageCapture().PotencyDef, EvalParams, Potency);
	Potency = FMath::Max(0.f, Potency);

	// Defense = how much damage the target can mitigate.
	float Defense = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		GetSpellDamageCapture().DefenseDef, EvalParams, Defense);
	Defense = FMath::Max(0.f, Defense);

	// ------------------------------------------------------------------
	// STEP 3: Read the Magnitude from the GE spec's Level.
	// This is the value your Spell blueprint plugs in from the data table's
	// EffectsDefStructure.Magnitude into the Level pin of MakeOutgoingGameplayEffectSpec.
	// ------------------------------------------------------------------
	const float Magnitude = Spec.GetLevel();

	// ------------------------------------------------------------------
	// STEP 4: Get the spell's tags directly from the SpellData table.
	// We read the caster's SelectedSpell string from AMyPawn, then look up
	// that row in the data table and extract the SpellTags container.
	// ------------------------------------------------------------------
	FGameplayTagContainer SpellTags;

	// Get the source actor (the caster).
	const UAbilitySystemComponent* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();
	if (SourceASC)
	{
		AMyPawn* CasterPawn = Cast<AMyPawn>(SourceASC->GetAvatarActor());
		if (CasterPawn)
		{
			// Load the SpellData data table.
			const UDataTable* SpellDataTable = Cast<UDataTable>(
				StaticLoadObject(UDataTable::StaticClass(), nullptr, *SpellDataTablePath));

			if (SpellDataTable)
			{
				SpellTags = GetSpellTagsFromDataTable(SpellDataTable, CasterPawn->SelectedSpell);
			}
		}
	}

	// ------------------------------------------------------------------
	// STEP 5: CORE DAMAGE FORMULA
	// ------------------------------------------------------------------
	//
	// TUNING CONSTANTS (change these to balance your game):
	const float PotencyScalar = 0.5f;      // How much each point of Potency contributes.
	const float DefenseConstant = 100.f;    // Higher = defense is less effective per point.
	//
	// At default stats (Potency=100, Defense=100, Magnitude=2):
	//   BaseDamage       = 2 * (100 * 0.5) = 100
	//   DefenseReduction = 100 / (100+100)  = 0.50  (50% damage blocked)
	//   PostDefenseDmg   = 100 * (1 - 0.5)  = 50   (5% of 1000 HP)
	//
	// Scaling examples:
	//   Defense 200 -> blocks 66.7%  (diminishing returns — good)
	//   Defense 400 -> blocks 80%    (still can't reach 100%)
	//   Potency 200 -> doubles raw damage (linear scaling — rewarding)
	//   Magnitude 4  -> doubles raw damage (data table tuning knob)
	//

	const float BaseDamage = Magnitude * (Potency * PotencyScalar);
	const float DefenseReduction = Defense / (Defense + DefenseConstant);
	float Damage = BaseDamage * (1.f - DefenseReduction);

	// ------------------------------------------------------------------
	// STEP 6: SPELL TAG SWITCHES
	// Each spell can have unique gameplay tags in its data table row.
	// When a tag is present, that spell's custom logic activates.
	//
	// HOW TO ADD A NEW SPELL:
	//   1. Create a gameplay tag (e.g., "Spell.MySpell")
	//   2. Add it to the spell's SpellTags in the data table.
	//   3. Copy one of the blocks below and change the tag + logic.
	// ------------------------------------------------------------------

	// --- FFD (Fireball) example: bonus damage vs OnFire and Vulnerable targets ---
	static const FGameplayTag SpellTag_FFD = FGameplayTag::RequestGameplayTag(FName("Spell.FFD"));
	if (SpellTags.HasTag(SpellTag_FFD))
	{
		// +30% bonus damage if the target is on fire (burning).
		static const FGameplayTag Debuff_OnFire = FGameplayTag::RequestGameplayTag(FName("Debuff.OnFire"));
		if (TargetTags && TargetTags->HasTag(Debuff_OnFire))
		{
			Damage *= 1.30f;
		}

		// +50% bonus damage if the target is vulnerable.
		// This stacks with the defense reduction that Vulnerable already applies.
		static const FGameplayTag Debuff_Vulnerable = FGameplayTag::RequestGameplayTag(FName("Debuff.Vulnerable"));
		if (TargetTags && TargetTags->HasTag(Debuff_Vulnerable))
		{
			Damage *= 1.50f;
		}
	}

	// ------------------------------------------------------------------
	// STEP 7: GLOBAL BUFF / DEBUFF MULTIPLIERS
	// These apply to ALL spells regardless of spell tags.
	// Good for buffs, roguelike rewards, or status effects.
	// ------------------------------------------------------------------

	// Buff.Charged on the caster -> +25% damage.
	// static const FGameplayTag Buff_Charged = FGameplayTag::RequestGameplayTag(FName("Buff.Charged"));
	// if (SourceTags && SourceTags->HasTag(Buff_Charged))
	// {
	// 	Damage *= 1.25f;
	// }

	// Buff.Honed on the caster -> +15% damage.
	// static const FGameplayTag Buff_Honed = FGameplayTag::RequestGameplayTag(FName("Buff.Honed"));
	// if (SourceTags && SourceTags->HasTag(Buff_Honed))
	// {
	// 	Damage *= 1.15f;
	// }

	// Debuff.Weakened on the target -> target takes +20% damage.
	// static const FGameplayTag Debuff_Weakened = FGameplayTag::RequestGameplayTag(FName("Debuff.Weakened"));
	// if (TargetTags && TargetTags->HasTag(Debuff_Weakened))
	// {
	// 	Damage *= 1.20f;
	// }

	// ------------------------------------------------------------------
	// STEP 8: FINAL OUTPUT
	// Apply the damage as a negative modifier to the target's Health.
	// ------------------------------------------------------------------
	Damage = FMath::Max(Damage, 0.f);

	if (Damage > 0.f)
	{
		OutExecutionOutput.AddOutputModifier(
			FGameplayModifierEvaluatedData(
				GetSpellDamageCapture().HealthProperty,
				EGameplayModOp::Additive,
				-Damage  // Negative = deals damage
			)
		);
	}
}
