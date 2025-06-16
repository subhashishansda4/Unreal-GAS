# Aura GAS System Flow Overview

This document provides an overview of the gameplay logic and architecture for the Aura project using Unreal Engine's Gameplay Ability System (GAS).

---

## 🎮 GAME START → PLAYER INITIALIZATION

**[GameModeBase::StartPlay]**
```
→ Spawns Pawn: AuraCharacter (inherits AuraCharacterBase)
→ AuraCharacter::BeginPlay()
    ├─ Initialize GAS via:
    │     • AuraAbilitySystemComponent
    │     • AuraAttributeSet
    ├─ AuraAbilitySystemLibrary::InitializeDefaultAttributes()
    │     → Applies GEs:
    │         • GE_PrimaryAttributes
    │         • GE_SecondaryAttributes
    │         • GE_VitalAttributes
    │     → Uses values from:
    │         • DA_CharacterClassInfo
    │         • CT_InitialPrimaryValues
    ├─ Setup Player HUD:
    │     • AuraHUD → CreateWidget<AuraUserWidget> (WBP_Overlay)
    │     • Assigns OverlayWidgetController to widget
    │     • WidgetController binds to:
    │         - AuraPlayerState
    │         - AuraAbilitySystemComponent
    │         - AuraAttributeSet
    └─ Bind Attribute Callbacks (Health, Mana, XP)
```

---

## 🧙 PLAYER INPUT → ABILITY ACTIVATION

**[Player Input Detected → Enhanced Input System]**
```
→ AuraInputComponent::BindAction("IA_LMB")
→ AuraPlayerController::OnInputActionLMB()
→ AbilitySystemComponent->TryActivateAbilityByTag("Ability.Firebolt")
→ GA_FireBolt (Blueprint GameplayAbility)
    ├─ Commit Ability
    │     • Applies GE_Cooldown_FireBolt, GE_Cost_FireBolt
    │     • Cost from CT_AbilityCostCurve
    ├─ Spawn Projectile (BP_FireBolt → AuraProjectile)
    │     • On impact → AuraAbilitySystemLibrary::ApplyDamageEffect()
    └─ Apply GE_Damage
          → Uses ExecCalc_Damage (C++)
          → Pulls from attacker & target attributes and CT_DamageCalculationCoefficients
```

---

## 💥 DAMAGE APPLICATION → ATTRIBUTE SET → UI RESPONSE

```
→ GE_Damage modifies AuraAttributeSet::Health
→ Server value changes → OnRep_Health() on clients
→ OverlayWidgetController::OnHealthChanged()
    → Updates WBP_HealthGlobe & WBP_FloatingDamageText

→ Crit or Block check in ExecCalc_Damage
    → GameplayCue triggered (e.g. Cue.Hit.Crit → Niagara VFX)
```

---

## 🧠 ENEMY AI BEHAVIOR FLOW

**[BP_Demon (inherits AuraEnemy) Spawned]**
```
→ AuraEnemy::PossessedBy(AuraAIController)
→ Run BT_EnemyBehaviorTree
    ├─ BTS_FindNearestPlayer → Sets TargetPlayer
    ├─ BTT_Attack → TryActivateAbility("Ability.Melee")
    ├─ BTT_GoAroundTarget
    └─ EQ_FindRangedAttackPosition
```

---

## 🌿 ENVIRONMENTAL INTERACTION

**[BP_HealthPotion → AuraEffectActor]**
```
→ Player overlaps → AuraEffectActor::OnOverlap()
→ ApplyEffectToTarget(Player, GE_PotionHeal)
    → Heal from CT_Potion (BaseHeal + Coefficient * Level)
→ Health Modified → OnRep_Health → WBP_HealthGlobe
```

**[BP_FireArea → AOE Zone]**
```
→ Applies GE_AreaFire every X seconds to overlapping actors
```

---

## 💀 CHARACTER DEATH LOGIC

**[Health <= 0 → AuraCharacterBase::HandleDeath()]**
```
→ MulticastHandleDeath()
→ Disable input, collisions
→ Play tagged death montage (CombatInterface)
→ Dissolve material effect
→ Notify UI to fade out

→ AuraEnemy::Die()
    → Unpossess AI Controller, stop BT
```

---

**End of Flow Documentation**
