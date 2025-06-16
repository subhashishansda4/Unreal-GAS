GAME START → PLAYER INITIALIZATION

[GameModeBase::StartPlay]
    ↓
[Spawns Pawn → AuraCharacter (inherits AuraCharacterBase)]
    ↓
[AuraCharacter::BeginPlay()]
    ├─ Initialize GAS via:
    │     AuraAbilitySystemComponent
    │     AuraAttributeSet
    │
    ├─ Call: AuraAbilitySystemLibrary::InitializeDefaultAttributes()
    │     → Applies GEs:
    │         ├─ GE_PrimaryAttributes
    │         ├─ GE_SecondaryAttributes
    │         └─ GE_VitalAttributes
    │     → Each GE uses values from:
    │         ├─ DA_CharacterClassInfo
    │         └─ CT_InitialPrimaryValues
    │
    ├─ Setup Player HUD:
    │     AuraHUD → CreateWidget<AuraUserWidget> (e.g. WBP_Overlay)
    │     → Assigns OverlayWidgetController to widget
    │     → WidgetController binds to:
    │         ├─ AuraPlayerState
    │         ├─ AuraAbilitySystemComponent
    │         └─ AuraAttributeSet
    │
    └─ Bind Attribute Callbacks:
          OverlayWidgetController → Listen to:
              OnAttributeChanged(Health, Mana, XP)


----------------------------------------------------------------------------------------------------------

PLAYER INPUT → ABILITY ACTIVATION

[Player Input Detected]
    ↓
[Enhanced Input System]
    → AuraInputComponent::BindAction("IA_LMB")
    ↓
[AuraPlayerController::OnInputActionLMB()]
    → Calls: AbilitySystemComponent->TryActivateAbilityByTag("Ability.Firebolt")
    ↓
[GA_FireBolt (Blueprint GameplayAbility)]
    ├─ Commit Ability
    │     → Applies:
    │         ├─ GE_Cooldown_FireBolt
    │         └─ GE_Cost_FireBolt (Mana cost)
    │     → GE_Cost_Firebolt uses:
    │         └─ CT_AbilityCostCurve
    │
    ├─ Spawn Projectile (BP_FireBolt → AuraProjectile)
    │     → Sets homing or directional velocity
    │     → On impact → calls AuraAbilitySystemLibrary::ApplyDamageEffect()
    │
    └─ Apply GE_Damage:
          → Uses ExecCalc_Damage (C++)
          → Pulls from:
              ├─ Attacker's Strength, CritChance, CritDamage
              ├─ Target's Armor
              └─ CT_DamageCalculationCoefficients
          → Result = Final Damage float


----------------------------------------------------------------------------------------------------------

DAMAGE APPLICATION → ATTRIBUTE SET → UI RESPONSE

[Target receives GE_Damage]
    ↓
[GE triggers modification to AuraAttributeSet::Health]
    → Value changes on server
    ↓
    [OnRep_Health()] triggered on clients
        ↓
    [OverlayWidgetController::OnHealthChanged()]
        → Broadcasts to bound UI widgets:
            ├─ WBP_HealthGlobe
            ├─ WBP_FloatingDamageText
        ↓
    → Health bar animates + Damage popup shown

[Critical or Blocked hits]
    ├─ ExecCalc_Damage checks:
        ├─ If Crit → Play FX, use CritDamageMult
        └─ If Blocked → Use reduced damage
    ↓
[GameplayCue Triggered] → e.g. "Cue.Hit.Crit" spawns Niagara VFX


---------------------------------------------------------------------------------------------------------

ENEMY AI BEHAVIOR FLOW

[BP_Demon (inherits AuraEnemy) spawned]
    ↓
[AuraEnemy::PossessedBy(AuraAIController)]
    → AIController::RunBehaviorTree(BT_EnemyBehaviorTree)
    ↓
[BT_EnemyBehaviorTree]
    ├─ Service: BTS_FindNearestPlayer
    │     → Sets TargetPlayer in Blackboard
    │
    ├─ Task: BTT_Attack
    │     → Checks range from player
    │     → If in range:
    │         → Calls AbilitySystemComponent->TryActivateAbility("Ability.Melee")
    │         → Follows same ability execution flow as player
    │
    ├─ Task: BTT_GoAroundTarget (Flanking)
    └─ EQS: EQ_FindRangedAttackPosition (for ranged logic)


---------------------------------------------------------------------------------------------------------

ENVIRONMENTAL INTERACTION

[BP_HealthPotion → Inherits AuraEffectActor]
    ↓
[Player Overlaps Actor]
    → AuraEffectActor::OnOverlap()
        → Calls ApplyEffectToTarget(Player, GE_PotionHeal)
            ↓
        [GE_PotionHeal]
            → Modifies Health based on CurveTable "CT_Potion"
            → FinalValue = BaseHeal + Coefficient * Level
    ↓
[Health Modified]
    → OnRep_Health() → OverlayWidgetController → WBP_HealthGlobe

[BP_FireArea → AOE Zone]
    → Constantly applies GE_AreaFire every X seconds to overlapping targets


---------------------------------------------------------------------------------------------------------

CHARACTER DEATH LOGIC

[Health <= 0 detected in AuraAttributeSet]
    ↓
[AuraCharacterBase::HandleDeath()]
    ├─ MulticastHandleDeath() → replicated to all clients
    ├─ Disables collisions + input
    ├─ Gets montage via CombatInterface::GetTaggedMontageByTag("Death")
    ├─ Plays death animation
    ├─ Starts dissolve material effect
    └─ Notifies WidgetController → Hide or Fade Out UI

[AuraEnemy::Die()]
    → Also unpossesses AI Controller
    → Stops BT execution
