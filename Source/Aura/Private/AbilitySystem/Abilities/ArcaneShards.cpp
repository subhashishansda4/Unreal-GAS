// Copyright Resurrect Studios. All Rights Reserved.


#include "AbilitySystem/Abilities/ArcaneShards.h"

FString UArcaneShards::GetDescription(int32 Level)
{
    const int32 ScaledDamage = Damage.GetValueAtLevel(Level);
    const int32 Shards = FMath::Min(Level, MaxNumShards);
    const int32 Debuff = DebuffChance;
    const float ManaCost = FMath::Abs(GetManaCost(Level));
    const float Cooldown = GetCooldown(Level);

    if (Level == 1)
    {
        return FString::Printf(TEXT(
                                   "<Title>ARCANE SHARDS</>\n\n"
                                   "<Small>Level: </><Level>%d</>\n"
                                   "<Small>Damage: </><Damage>%d</>\n"
                                   "<Small>Shards: </><Time>%d</>\n"
                                   "<Small>Debuff Chance: </><Percent>%d%%</>\n"
                                   "<Small>Mana Cost: </><ManaCost>%.1f</>\n"
                                   "<Small>Cooldown: </><Cooldown>%.1f</>\n\n"
                                   "<Small>Summon a shard of arcane energy causing radial damage</>"),
                               Level, ScaledDamage, Shards, Debuff, ManaCost, Cooldown);
    }
    else
    {
        return FString::Printf(TEXT(
                                   "<Title>ARCANE SHARDS</>\n\n"
                                   "<Small>Level: </><Level>%d</>\n"
                                   "<Small>Damage: </><Damage>%d</>\n"
                                   "<Small>Shards: </><Time>%d</>\n"
                                   "<Small>Debuff Chance: </><Percent>%d%%</>\n"
                                   "<Small>Mana Cost: </><ManaCost>%.1f</>\n"
                                   "<Small>Cooldown: </><Cooldown>%.1f</>\n\n"
                                   "<Small>Summons %d shards of arcane energy causing radial damage</>"),
                               Level, ScaledDamage, Shards, Debuff, ManaCost, Cooldown, Shards);
    }
}

FString UArcaneShards::GetNextLevelDescription(int32 Level)
{
    const int32 ScaledDamage = Damage.GetValueAtLevel(Level);
    const int32 Shards = FMath::Min(Level, MaxNumShards);
    const int32 Debuff = DebuffChance;
    const float ManaCost = FMath::Abs(GetManaCost(Level));
    const float Cooldown = GetCooldown(Level);

    return FString::Printf(TEXT(
                               "<Title>NEXT LEVEL</>\n\n"
                               "<Small>Level: </><Level>%d</>\n"
                               "<Small>Damage: </><Damage>%d</>\n"
                               "<Small>Shards: </><Time>%d</>\n"
                               "<Small>Debuff Chance: </><Percent>%d%%</>\n"
                               "<Small>Mana Cost: </><ManaCost>%.1f</>\n"
                               "<Small>Cooldown: </><Cooldown>%.1f</>\n\n"
                               "<Small>Summons %d shards of arcane energy causing radial damage</>"),
                           Level, ScaledDamage, Shards, Debuff, ManaCost, Cooldown, Shards);
}