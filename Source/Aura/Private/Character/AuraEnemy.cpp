// Copyright Resurrect Studios

#include "Character/AuraEnemy.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AbilitySystem/AuraAttributeSet.h"
#include "Aura/Aura.h"
#include "Components/WidgetComponent.h"
#include "UI/Widget/AuraUserWidget.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "AuraGameplayTags.h"
#include "AI/AuraAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AAuraEnemy::AAuraEnemy()
{
    GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

    AbilitySystemComponent = CreateDefaultSubobject<UAuraAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
    AbilitySystemComponent->SetIsReplicated(true);
    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

    bUseControllerRotationPitch = false;
    bUseControllerRotationRoll = false;
    bUseControllerRotationYaw = false;
    GetCharacterMovement()->bUseControllerDesiredRotation = true;

    AttributeSet = CreateDefaultSubobject<UAuraAttributeSet>(TEXT("AttributeSet"));

    HealthBar = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBar"));
    HealthBar->SetupAttachment(GetRootComponent());

    GetMesh()->SetCustomDepthStencilValue(CUSTOM_DEPTH_RED);
    GetMesh()->MarkRenderStateDirty();
    Weapon->SetCustomDepthStencilValue(CUSTOM_DEPTH_RED);
    Weapon->MarkRenderStateDirty();

    BaseWalkSpeed = 250.f;
}

void AAuraEnemy::PossessedBy(AController *NewController)
{
    Super::PossessedBy(NewController);

    if (!HasAuthority())
        return;
    AuraAIController = Cast<AAuraAIController>(NewController);
    AuraAIController->GetBlackboardComponent()->InitializeBlackboard(*BehaviorTree->BlackboardAsset);
    AuraAIController->RunBehaviorTree(BehaviorTree);
    AuraAIController->GetBlackboardComponent()->SetValueAsBool(FName("HitReacting"), false);
    AuraAIController->GetBlackboardComponent()->SetValueAsBool(FName("RangedAttacker"), CharacterClass != ECharacterClass::Warrior);
}

void AAuraEnemy::HighlightActor_Implementation()
{
    GetMesh()->SetRenderCustomDepth(true);
    Weapon->SetRenderCustomDepth(true);
}

void AAuraEnemy::UnHighlightActor_Implementation()
{
    GetMesh()->SetRenderCustomDepth(false);
    Weapon->SetRenderCustomDepth(false);
}

void AAuraEnemy::SetMoveToLocation_Implementation(FVector &OutDestination)
{
    //  do not change OutDestination
}

int32 AAuraEnemy::GetPlayerLevel_Implementation()
{
    return Level;
}

void AAuraEnemy::Die(const FVector &DeathImpulse)
{
    SetLifeSpan(LifeSpan);
    if (AuraAIController)
        AuraAIController->GetBlackboardComponent()->SetValueAsBool(FName("Dead"), true);
    Super::Die(DeathImpulse);
}

void AAuraEnemy::SetCombatTarget_Implementation(AActor *InCombatTarget)
{
    CombatTarget = InCombatTarget;
}

AActor *AAuraEnemy::GetCombatTarget_Implementation() const
{
    return CombatTarget;
}

void AAuraEnemy::BeginPlay()
{
    Super::BeginPlay();

    GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;

    InitAbilityActorInfo();

    if (HasAuthority())
    {
        UAuraAbilitySystemLibrary::GiveStartupAbilities(this, AbilitySystemComponent, CharacterClass);
    }

    if (UAuraUserWidget *AuraUserWidget = Cast<UAuraUserWidget>(HealthBar->GetUserWidgetObject()))
    {
        AuraUserWidget->SetWidgetController(this);
    }

    if (const UAuraAttributeSet *AuraAS = CastChecked<UAuraAttributeSet>(AttributeSet))
    {
        AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AuraAS->GetHealthAttribute()).AddLambda([this](const FOnAttributeChangeData &Data)
                                                                                                                { OnHealthChanged.Broadcast(Data.NewValue); });
        AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AuraAS->GetMaxHealthAttribute()).AddLambda([this](const FOnAttributeChangeData &Data)
                                                                                                                   { OnMaxHealthChanged.Broadcast(Data.NewValue); });
        AbilitySystemComponent->RegisterGameplayTagEvent(FAuraGameplayTags::Get().Effects_HitReact, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &AAuraEnemy::HitReactTagChanged);

        OnHealthChanged.Broadcast(AuraAS->GetHealth());
        OnMaxHealthChanged.Broadcast(AuraAS->GetMaxHealth());
    }
}

void AAuraEnemy::HitReactTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
    bHitReacting = NewCount > 0;
    GetCharacterMovement()->MaxWalkSpeed = bHitReacting ? 0.f : BaseWalkSpeed;
    if (AuraAIController && AuraAIController->GetBlackboardComponent())
    {
        AuraAIController->GetBlackboardComponent()->SetValueAsBool(FName("HitReacting"), bHitReacting);
    }
}

void AAuraEnemy::InitAbilityActorInfo()
{
    AbilitySystemComponent->InitAbilityActorInfo(this, this);
    Cast<UAuraAbilitySystemComponent>(AbilitySystemComponent)->AbilityActorInfoSet();

    AbilitySystemComponent->RegisterGameplayTagEvent(FAuraGameplayTags::Get().Debuff_Shock, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &AAuraEnemy::ShockTagChanged);

    if (HasAuthority())
    {
        InitializeDefaultAttributes();
    }

    OnASCRegistered.Broadcast(AbilitySystemComponent);
}

void AAuraEnemy::InitializeDefaultAttributes() const
{
    UAuraAbilitySystemLibrary::InitializeDefaultAttributes(this, CharacterClass, Level, AbilitySystemComponent);
}

void AAuraEnemy::ShockTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
    Super::ShockTagChanged(CallbackTag, NewCount);

    if (AuraAIController && AuraAIController->GetBlackboardComponent())
    {
        AuraAIController->GetBlackboardComponent()->SetValueAsBool(FName("Shocked"), bIsShocked);
    }
}