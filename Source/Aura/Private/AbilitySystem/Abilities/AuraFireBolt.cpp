// Copyright Resurrect Studios. All Rights Reserved.

#include "AbilitySystem/Abilities/AuraFireBolt.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "Actor/AuraProjectile.h"
#include "GameFramework/ProjectileMovementComponent.h"

FString UAuraFireBolt::GetDescription(int32 Level)
{
    const int32 ScaledDamage = Damage.GetValueAtLevel(Level);
    const int32 Projectiles = FMath::Min(Level, NumProjectiles);
    const int32 Debuff = DebuffChance;
    const float ManaCost = FMath::Abs(GetManaCost(Level));
    const float Cooldown = GetCooldown(Level);

    if (Level == 1)
    {
        return FString::Printf(TEXT(
                                   "<Title>FIRE BOLT</>\n\n"
                                   "<Small>Level: </><Level>%d</>\n"
                                   "<Small>Damage: </><Damage>%d</>\n"
                                   "<Small>Projectile: </><Time>%d</>\n"
                                   "<Small>Burn Chance: </><Percent>%d%%</>\n"
                                   "<Small>Mana Cost: </><ManaCost>%.1f</>\n"
                                   "<Small>Cooldown: </><Cooldown>%.1f</>\n\n"
                                   "<Small>Fires a projectile towards a target dealing fire damage</>"),
                               Level, ScaledDamage, Projectiles, Debuff, ManaCost, Cooldown);
    }
    else
    {
        return FString::Printf(TEXT(
                                   "<Title>FIRE BOLT</>\n\n"
                                   "<Small>Level: </><Level>%d</>\n"
                                   "<Small>Damage: </><Damage>%d</>\n"
                                   "<Small>Projectiles: </><Time>%d</>\n"
                                   "<Small>Burn Chance: </><Percent>%d%%</>\n"
                                   "<Small>Mana Cost: </><ManaCost>%.1f</>\n"
                                   "<Small>Cooldown: </><Cooldown>%.1f</>\n\n"
                                   "<Small>Fires %d projectiles towards a target dealing fire damage</>"),
                               Level, ScaledDamage, Projectiles, Debuff, ManaCost, Cooldown, Projectiles);
    }
}

FString UAuraFireBolt::GetNextLevelDescription(int32 Level)
{
    const int32 ScaledDamage = Damage.GetValueAtLevel(Level);
    const int32 Projectiles = FMath::Min(Level, NumProjectiles);
    const int32 Debuff = DebuffChance;
    const float ManaCost = FMath::Abs(GetManaCost(Level));
    const float Cooldown = GetCooldown(Level);

    return FString::Printf(TEXT(
                               "<Title>NEXT LEVEL</>\n\n"
                               "<Small>Level: </><Level>%d</>\n"
                               "<Small>Damage: </><Damage>%d</>\n"
                               "<Small>Projectiles: </><Time>%d</>\n"
                               "<Small>Burn Chance: </><Percent>%d%%</>\n"
                               "<Small>Mana Cost: </><ManaCost>%.1f</>\n"
                               "<Small>Cooldown: </><Cooldown>%.1f</>\n\n"
                               "<Small>Fires %d projectiles towards a target dealing fire damage</>"),
                           Level, ScaledDamage, Projectiles, Debuff, ManaCost, Cooldown, Projectiles);
}

void UAuraFireBolt::SpawnProjectiles(const FVector &ProjectileTargetLocation, const FGameplayTag &SocketTag, bool bOverridePitch, float PitchOverride, AActor *HomingTarget)
{
    const bool bIsServer = GetAvatarActorFromActorInfo()->HasAuthority();
    if (!bIsServer) return;

    const FVector SocketLocation = ICombatInterface::Execute_GetCombatSocketLocation(GetAvatarActorFromActorInfo(), SocketTag);
    FRotator Rotation = (ProjectileTargetLocation - SocketLocation).Rotation();
    if (bOverridePitch) Rotation.Pitch = PitchOverride;

    const FVector Forward = Rotation.Vector();
    const int32 EffectiveNumProjectiles = FMath::Min(NumProjectiles, GetAbilityLevel());

    TArray<FRotator> Rotations = UAuraAbilitySystemLibrary::EvenlySpacedRotators(Forward, FVector::UpVector, ProjectileSpread, EffectiveNumProjectiles);

    for (const FRotator &Rot : Rotations)
    {
        FTransform SpawnTransform;
        SpawnTransform.SetLocation(SocketLocation);
        SpawnTransform.SetRotation(Rot.Quaternion());

        AAuraProjectile *Projectile = GetWorld()->SpawnActorDeferred<AAuraProjectile>(ProjectileClass, SpawnTransform, GetOwningActorFromActorInfo(), Cast<APawn>(GetOwningActorFromActorInfo()), ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

        Projectile->DamageEffectParams = MakeDamageEffectParamsFromClassDefaults();

        if (GetAbilityLevel() > 1)
        {
            Projectile->HomingTargetSceneComponent = NewObject<USceneComponent>(USceneComponent::StaticClass());
            Projectile->HomingTargetSceneComponent->SetWorldLocation(ProjectileTargetLocation);
            Projectile->ProjectileMovement->HomingTargetComponent = Projectile->HomingTargetSceneComponent;
            Projectile->ProjectileMovement->HomingAccelerationMagnitude = FMath::FRandRange(HomingAccelerationMin, HomingAccelerationMax);
            Projectile->ProjectileMovement->bIsHomingProjectile = bLaunchHomingProjectiles;
            Projectile->ProjectileMovement->ProjectileGravityScale = 0.3f;

            if (HomingTarget->Implements<UCombatInterface>())
            {
                Projectile->ProjectileMovement->HomingTargetComponent = HomingTarget->GetRootComponent();
            }
        }
        else
        {
            Projectile->ProjectileMovement->ProjectileGravityScale = 0.f;
            Projectile->ProjectileMovement->bIsHomingProjectile = !bLaunchHomingProjectiles;
        }

        Projectile->FinishSpawning(SpawnTransform);
    }
    
}
