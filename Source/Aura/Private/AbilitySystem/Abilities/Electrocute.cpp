// Copyright Resurrect Studios. All Rights Reserved.


#include "AbilitySystem/Abilities/Electrocute.h"

FString UElectrocute::GetDescription(int32 Level)
{
    const int32 ScaledDamage = Damage.GetValueAtLevel(Level);
    const int32 Chains = FMath::Min(Level, MaxNumShockTargets);
    const int32 Debuff = DebuffChance;
    const float ManaCost = FMath::Abs(GetManaCost(Level));
    const float Cooldown = GetCooldown(Level);

    if (Level == 1)
    {
        return FString::Printf(TEXT(
                                   "<Title>ELECTROCUTE</>\n\n"
                                   "<Small>Level: </><Level>%d</>\n"
                                   "<Small>Damage: </><Damage>%d</>\n"
                                   "<Small>Chains: </><Time>%d</>\n"
                                   "<Small>Shock Chance: </><Percent>%d%%</>\n"
                                   "<Small>Mana Cost: </><ManaCost>%.1f</>\n"
                                   "<Small>Cooldown: </><Cooldown>%.1f</>\n\n"
                                   "<Small>Emits a beam of lightning connecting with the target</>"),
                               Level, ScaledDamage, Chains, Debuff, ManaCost, Cooldown);
    }
    else
    {
        return FString::Printf(TEXT(
                                   "<Title>ELECTROCUTE</>\n\n"
                                   "<Small>Level: </><Level>%d</>\n"
                                   "<Small>Damage: </><Damage>%d</>\n"
                                   "<Small>Chain: </><Time>%d</>\n"
                                   "<Small>Shock Chance: </><Percent>%d%%</>\n"
                                   "<Small>Mana Cost: </><ManaCost>%.1f</>\n"
                                   "<Small>Cooldown: </><Cooldown>%.1f</>\n\n"
                                   "<Small>Emits a beam of lightning connecting with the target propagating to %d additional targets/>"),
                               Level, ScaledDamage, Chains, Debuff, ManaCost, Cooldown, Chains);
    }
}

FString UElectrocute::GetNextLevelDescription(int32 Level)
{
    const int32 ScaledDamage = Damage.GetValueAtLevel(Level);
    const int32 Chains = FMath::Min(Level, MaxNumShockTargets);
    const int32 Debuff = DebuffChance;
    const float ManaCost = FMath::Abs(GetManaCost(Level));
    const float Cooldown = GetCooldown(Level);

    return FString::Printf(TEXT(
                               "<Title>NEXT LEVEL</>\n\n"
                               "<Small>Level: </><Level>%d</>\n"
                               "<Small>Damage: </><Damage>%d</>\n"
                               "<Small>Chain: </><Time>%d</>\n"
                               "<Small>Shock Chance: </><Percent>%d%%</>\n"
                               "<Small>Mana Cost: </><ManaCost>%.1f</>\n"
                               "<Small>Cooldown: </><Cooldown>%.1f</>\n\n"
                               "<Small>Emits a beam of lightning connecting with the target propagating to %d additional targets</>"),
                           Level, ScaledDamage, Chains, Debuff, ManaCost, Cooldown, Chains);
}