// Copyright Resurrect Studios

#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AuraGameplayTags.h"
#include "AbilitySystem/Abilities/AuraGameplayAbility.h"
#include "Aura/AuraLogChannels.h"
#include "Interaction/PlayerInterface.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "AbilitySystem/Data/AbilityInfo.h"
#include "Game/LoadScreenSaveGame.h"

void UAuraAbilitySystemComponent::AbilityActorInfoSet()
{
    OnGameplayEffectAppliedDelegateToSelf.AddUObject(this, &UAuraAbilitySystemComponent::ClientEffectApplied);
}

void UAuraAbilitySystemComponent::AddCharacterAbilitiesFromSaveData(ULoadScreenSaveGame *SaveData)
{
    for (const FSavedAbility &Data : SaveData->SavedAbilities)
    {
        const TSubclassOf<UGameplayAbility> LoadedAbilityClass = Data.GameplayAbility;

        FGameplayAbilitySpec LoadedAbilitySpec = FGameplayAbilitySpec(LoadedAbilityClass, Data.AbilityLevel);
        
        LoadedAbilitySpec.GetDynamicSpecSourceTags().AddTag(Data.AbilitySlot);
        LoadedAbilitySpec.GetDynamicSpecSourceTags().AddTag(Data.AbilityStatus);

        if (Data.AbilityType == FAuraGameplayTags::Get().Abilities_Type_Offensive)
        {
            GiveAbility(LoadedAbilitySpec);
        }
        else if (Data.AbilityType == FAuraGameplayTags::Get().Abilities_Type_Passive)
        {
            if (Data.AbilityStatus.MatchesTagExact(FAuraGameplayTags::Get().Abilities_Status_Equipped))
            {
                GiveAbilityAndActivateOnce(LoadedAbilitySpec);
            }
            else
            {
                GiveAbility(LoadedAbilitySpec);
            }
        }
    }
    bStartupAbilitiesGiven = true;
    AbilitiesGivenDelegate.Broadcast();
}

void UAuraAbilitySystemComponent::AddCharacterAbilities(const TArray<TSubclassOf<UGameplayAbility>> &StartupAbilities)
{
    for (const TSubclassOf<UGameplayAbility> AbilityClass : StartupAbilities)
    {
        FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(AbilityClass, 1);
        if (const UAuraGameplayAbility *AuraAbility = Cast<UAuraGameplayAbility>(AbilitySpec.Ability))
        {
            AbilitySpec.GetDynamicSpecSourceTags().AddTag(AuraAbility->StartupInputTag);
            AbilitySpec.GetDynamicSpecSourceTags().AddTag(FAuraGameplayTags::Get().Abilities_Status_Equipped);
            GiveAbility(AbilitySpec);
        }
    }
    bStartupAbilitiesGiven = true;
    AbilitiesGivenDelegate.Broadcast();
}

void UAuraAbilitySystemComponent::AddCharacterPassiveAbilities(const TArray<TSubclassOf<UGameplayAbility>> &StartupPassiveAbilities)
{
    for (const TSubclassOf<UGameplayAbility> AbilityClass : StartupPassiveAbilities)
    {
        FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(AbilityClass, 1);
        AbilitySpec.GetDynamicSpecSourceTags().AddTag(FAuraGameplayTags::Get().Abilities_Status_Equipped);
        GiveAbilityAndActivateOnce(AbilitySpec);
    }
}

void UAuraAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag &InputTag)
{
    if (!InputTag.IsValid())
        return;

    FScopedAbilityListLock ActiveScopeLock(*this);
    for (FGameplayAbilitySpec &AbilitySpec : GetActivatableAbilities())
    {
        if (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
        {
            AbilitySpecInputPressed(AbilitySpec);
            if (AbilitySpec.IsActive())
            {
                UGameplayAbility *PrimaryInstance = AbilitySpec.GetPrimaryInstance();
                if (PrimaryInstance)
                {
                    InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, AbilitySpec.Handle, PrimaryInstance->GetCurrentActivationInfo().GetActivationPredictionKey());
                }
            }
        }
    }
}

void UAuraAbilitySystemComponent::AbilityInputTagHeld(const FGameplayTag &InputTag)
{
    if (!InputTag.IsValid())
        return;

    FScopedAbilityListLock ActiveScopeLock(*this);
    for (FGameplayAbilitySpec &AbilitySpec : GetActivatableAbilities())
    {
        if (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
        {
            AbilitySpecInputPressed(AbilitySpec);
            if (!AbilitySpec.IsActive())
            {
                TryActivateAbility(AbilitySpec.Handle);
            }
        }
    }
}

void UAuraAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag &InputTag)
{
    if (!InputTag.IsValid())
        return;

    FScopedAbilityListLock ActiveScopeLock(*this);
    for (FGameplayAbilitySpec &AbilitySpec : GetActivatableAbilities())
    {
        if (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
        {
            AbilitySpecInputReleased(AbilitySpec);
            if (AbilitySpec.IsActive())
            {
                UGameplayAbility *PrimaryInstance = AbilitySpec.GetPrimaryInstance();
                if (PrimaryInstance)
                {
                    InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, AbilitySpec.Handle, PrimaryInstance->GetCurrentActivationInfo().GetActivationPredictionKey());
                }
            }
        }
    }
}

void UAuraAbilitySystemComponent::ForEachAbility(const FForEachAbility &Delegate)
{
    FScopedAbilityListLock ActiveScopeLock(*this);
    for (const FGameplayAbilitySpec &AbilitySpec : GetActivatableAbilities())
    {
        if (!Delegate.ExecuteIfBound(AbilitySpec))
        {
            UE_LOG(LogAura, Error, TEXT("Failed to execute delegate in %hs"), __FUNCTION__);
        }
    }
}

FGameplayTag UAuraAbilitySystemComponent::GetAbilityTagFromSpec(const FGameplayAbilitySpec &AbilitySpec)
{
    if (AbilitySpec.Ability)
    {
        for (FGameplayTag Tag : AbilitySpec.Ability.Get()->GetAssetTags())
        {
            if (Tag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("Abilities"))))
            {
                return Tag;
            }
        }
    }
    return FGameplayTag();
}

FGameplayTag UAuraAbilitySystemComponent::GetInputTagFromSpec(const FGameplayAbilitySpec &AbilitySpec)
{
    for (FGameplayTag Tag : AbilitySpec.GetDynamicSpecSourceTags())
    {
        if (Tag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("InputTag"))))
        {
            return Tag;
        }
    }
    return FGameplayTag();
}

FGameplayTag UAuraAbilitySystemComponent::GetStatusFromSpec(const FGameplayAbilitySpec &AbilitySpec)
{
    for (FGameplayTag StatusTag : AbilitySpec.GetDynamicSpecSourceTags())
    {
        if (StatusTag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("Abilities.Status"))))
        {
            return StatusTag;
        }
    }
    return FGameplayTag();
}

FGameplayTag UAuraAbilitySystemComponent::GetStatusFromAbilityTag(const FGameplayTag &AbilityTag)
{
    if (const FGameplayAbilitySpec *Spec = GetSpecFromAbilityTag(AbilityTag))
    {
        return GetStatusFromSpec(*Spec);
    }
    return FGameplayTag();
}

FGameplayTag UAuraAbilitySystemComponent::GetSlotFromAbilityTag(const FGameplayTag &AbilityTag)
{
    if (const FGameplayAbilitySpec *Spec = GetSpecFromAbilityTag(AbilityTag))
    {
        return GetInputTagFromSpec(*Spec);
    }
    return FGameplayTag();
}

bool UAuraAbilitySystemComponent::SlotIsEmpty(const FGameplayTag &Slot)
{
    FScopedAbilityListLock ActiveScopeLoc(*this);
    for (FGameplayAbilitySpec &AbilitySpec : GetActivatableAbilities())
    {
        if (AbilityHasSlot(AbilitySpec, Slot))
        {
            return false;
        }
    }
    return true;
}

bool UAuraAbilitySystemComponent::AbilityHasSlot(const FGameplayAbilitySpec &Spec, const FGameplayTag &Slot)
{
    return Spec.GetDynamicSpecSourceTags().HasTagExact(Slot);
}

bool UAuraAbilitySystemComponent::AbilityHasAnySlot(const FGameplayAbilitySpec &Spec)
{
    return Spec.GetDynamicSpecSourceTags().HasTag(FGameplayTag::RequestGameplayTag(FName("InputTag")));
}

FGameplayAbilitySpec *UAuraAbilitySystemComponent::GetSpecWithSlot(const FGameplayTag &Slot)
{
    FScopedAbilityListLock ActiveScopeLock(*this);
    for (FGameplayAbilitySpec &AbilitySpec : GetActivatableAbilities())
    {
        if (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(Slot))
        {
            return &AbilitySpec;
        }
    }
    return nullptr;
}

bool UAuraAbilitySystemComponent::IsPassiveAbility(const FGameplayAbilitySpec &Spec) const
{
    const UAbilityInfo *AbilityInfo = UAuraAbilitySystemLibrary::GetAbilityInfo(GetAvatarActor());
    const FGameplayTag AbilityTag = GetAbilityTagFromSpec(Spec);
    const FAuraAbilityInfo &Info = AbilityInfo->FindAbilityInfoForTag(AbilityTag);
    const FGameplayTag AbilityType = Info.AbilityType;
    return AbilityType.MatchesTagExact(FAuraGameplayTags::Get().Abilities_Type_Passive);
}

void UAuraAbilitySystemComponent::AssignSlotToAbility(FGameplayAbilitySpec &Spec, const FGameplayTag &Slot)
{
    ClearSlot(&Spec);
    Spec.GetDynamicSpecSourceTags().AddTag(Slot);
}

void UAuraAbilitySystemComponent::MulticastActivatePassiveEffect_Implementation(const FGameplayTag &AbilityTag, bool bActivate)
{
    ActivatePassiveEffect.Broadcast(AbilityTag, bActivate);
}

FGameplayAbilitySpec *UAuraAbilitySystemComponent::GetSpecFromAbilityTag(const FGameplayTag &AbilityTag)
{
    FScopedAbilityListLock ActiveScopeLock(*this);
    for (FGameplayAbilitySpec &AbilitySpec : GetActivatableAbilities())
    {
        for (FGameplayTag Tag : AbilitySpec.Ability.Get()->GetAssetTags())
        {
            if (Tag.MatchesTag(AbilityTag))
            {
                return &AbilitySpec;
            }
        }
    }
    return nullptr;
}

void UAuraAbilitySystemComponent::UpgradeAttribute(const FGameplayTag &AttributeTag)
{
    if (GetAvatarActor()->Implements<UPlayerInterface>())
    {
        if (IPlayerInterface::Execute_GetAttributePoints(GetAvatarActor()) > 0)
        {
            ServerUpgradeAttribute(AttributeTag);
        }
    }
}

void UAuraAbilitySystemComponent::ServerUpgradeAttribute_Implementation(const FGameplayTag &AttributeTag)
{
    FGameplayEventData Payload;
    Payload.EventTag = AttributeTag;
    Payload.EventMagnitude = 1.f;

    UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetAvatarActor(), AttributeTag, Payload);

    if (GetAvatarActor()->Implements<UPlayerInterface>())
    {
        IPlayerInterface::Execute_AddToAttributePoints(GetAvatarActor(), -1);
    }
}

void UAuraAbilitySystemComponent::UpdateAbilityStatuses(int32 Level)
{
    UAbilityInfo *AbilityInfo = UAuraAbilitySystemLibrary::GetAbilityInfo(GetAvatarActor());
    for (const FAuraAbilityInfo &Info : AbilityInfo->AbilityInformation)
    {
        if (!Info.AbilityTag.IsValid())
            continue;
        if (Level < Info.LevelRequirement)
            continue;
        if (GetSpecFromAbilityTag(Info.AbilityTag) == nullptr)
        {
            FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(Info.Ability, 1);
            AbilitySpec.GetDynamicSpecSourceTags().AddTag(FAuraGameplayTags::Get().Abilities_Status_Eligible);
            GiveAbility(AbilitySpec);
            MarkAbilitySpecDirty(AbilitySpec);
            ClientUpdateAbilityStatus(Info.AbilityTag, FAuraGameplayTags::Get().Abilities_Status_Eligible, 1);
        }
    }
}

void UAuraAbilitySystemComponent::ServerSpendSpellPoint_Implementation(const FGameplayTag &AbilityTag)
{
    if (FGameplayAbilitySpec *AbilitySpec = GetSpecFromAbilityTag(AbilityTag))
    {
        if (GetAvatarActor()->Implements<UPlayerInterface>())
        {
            IPlayerInterface::Execute_AddToSpellPoints(GetAvatarActor(), -1);
        }

        const FAuraGameplayTags GameplayTags = FAuraGameplayTags::Get();
        FGameplayTag Status = GetStatusFromSpec(*AbilitySpec);
        if (Status.MatchesTagExact(GameplayTags.Abilities_Status_Eligible))
        {
            AbilitySpec->GetDynamicSpecSourceTags().RemoveTag(GameplayTags.Abilities_Status_Eligible);
            AbilitySpec->GetDynamicSpecSourceTags().AddTag(GameplayTags.Abilities_Status_Unlocked);
            Status = GameplayTags.Abilities_Status_Unlocked;
        }
        else if (Status.MatchesTagExact(GameplayTags.Abilities_Status_Equipped) || Status.MatchesTagExact(GameplayTags.Abilities_Status_Unlocked))
        {
            AbilitySpec->Level += 1;
        }
        ClientUpdateAbilityStatus(AbilityTag, Status, AbilitySpec->Level);
        MarkAbilitySpecDirty(*AbilitySpec);
    }
}

void UAuraAbilitySystemComponent::ServerEquipAbility_Implementation(const FGameplayTag &AbilityTag, const FGameplayTag &Slot)
{
    if (FGameplayAbilitySpec *AbilitySpec = GetSpecFromAbilityTag(AbilityTag))
    {
        const FAuraGameplayTags &GameplayTags = FAuraGameplayTags::Get();
        const FGameplayTag &PrevSlot = GetInputTagFromSpec(*AbilitySpec);
        const FGameplayTag &Status = GetStatusFromSpec(*AbilitySpec);

        const bool bStatusValid = Status == GameplayTags.Abilities_Status_Equipped || Status == GameplayTags.Abilities_Status_Unlocked;
        if (bStatusValid)
        {
            // Handle activation/deactivation for passive abilities

            if (!SlotIsEmpty(Slot)) // there is an ability in this slot already. deactivate and clear its slot
            {
                FGameplayAbilitySpec *SpecWithSlot = GetSpecWithSlot(Slot);
                if (SpecWithSlot)
                {
                    // is that ability the same as this ability? if so, we can return early
                    if (AbilityTag.MatchesTagExact(GetAbilityTagFromSpec(*SpecWithSlot)))
                    {
                        ClientEquipAbility(AbilityTag, GameplayTags.Abilities_Status_Equipped, Slot, PrevSlot);
                        return;
                    }
                    
                    if (IsPassiveAbility(*SpecWithSlot))
                    {
                        MulticastActivatePassiveEffect(GetAbilityTagFromSpec(*SpecWithSlot), false);                        DeactivatePassiveAbility.Broadcast(GetAbilityTagFromSpec(*SpecWithSlot));
                    }

                    ClearSlot(SpecWithSlot);
                }
            }

            if (!AbilityHasAnySlot(*AbilitySpec)) // ability does not have any slot (it's not active)
            {
                if (IsPassiveAbility(*AbilitySpec))
                {
                    TryActivateAbility(AbilitySpec->Handle);
                    MulticastActivatePassiveEffect(AbilityTag, true);
                }
                AbilitySpec->GetDynamicSpecSourceTags().RemoveTag(GetStatusFromSpec(*AbilitySpec));
                AbilitySpec->GetDynamicSpecSourceTags().AddTag(GameplayTags.Abilities_Status_Equipped);
            }
            AssignSlotToAbility(*AbilitySpec, Slot);
            MarkAbilitySpecDirty(*AbilitySpec);
        }
        ClientEquipAbility(AbilityTag, GameplayTags.Abilities_Status_Equipped, Slot, PrevSlot);
    }
}

void UAuraAbilitySystemComponent::ClientEquipAbility_Implementation(const FGameplayTag &AbilityTag, const FGameplayTag &Status, const FGameplayTag &Slot, const FGameplayTag &PrevSlot)
{
    AbilityEquipped.Broadcast(AbilityTag, Status, Slot, PrevSlot);
}

bool UAuraAbilitySystemComponent::GetDescriptionsByAbilityTag(const FGameplayTag &AbilityTag, FString &OutDescription, FString &OutNextLevelDescription)
{
    if (const FGameplayAbilitySpec *AbilitySpec = GetSpecFromAbilityTag(AbilityTag))
    {
        if (UAuraGameplayAbility *AuraAbility = Cast<UAuraGameplayAbility>(AbilitySpec->Ability))
        {
            OutDescription = AuraAbility->GetDescription(AbilitySpec->Level);
            OutNextLevelDescription = AuraAbility->GetNextLevelDescription(AbilitySpec->Level + 1);
            return true;
        }
    }
    const UAbilityInfo *AbilityInfo = UAuraAbilitySystemLibrary::GetAbilityInfo(GetAvatarActor());
    if (!AbilityTag.IsValid() || AbilityTag.MatchesTagExact(FAuraGameplayTags::Get().Abilities_None))
    {
        OutDescription = FString();
    }
    else
    {
        OutDescription = UAuraGameplayAbility::GetLockedDescription(AbilityInfo->FindAbilityInfoForTag(AbilityTag).LevelRequirement);
    }
    OutNextLevelDescription = FString();
    return false;
}

void UAuraAbilitySystemComponent::ClearSlot(FGameplayAbilitySpec *Spec)
{
    const FGameplayTag &Slot = GetInputTagFromSpec(*Spec);
    Spec->GetDynamicSpecSourceTags().RemoveTag(Slot);
}

void UAuraAbilitySystemComponent::ClearAbilitiesOfSlot(const FGameplayTag &Slot)
{
    FScopedAbilityListLock AbilityScopeLock(*this);
    for (FGameplayAbilitySpec &Spec : GetActivatableAbilities())
    {
        if (AbilityHasSlot(&Spec, Slot))
        {
            ClearSlot(&Spec);
        }
    }
}

bool UAuraAbilitySystemComponent::AbilityHasSlot(FGameplayAbilitySpec *Spec, const FGameplayTag &Slot)
{
    for (FGameplayTag Tag : Spec->GetDynamicSpecSourceTags())
    {
        if (Tag.MatchesTagExact(Slot))
        {
            return true;
        }
    }
    return false;
}

void UAuraAbilitySystemComponent::OnRep_ActivateAbilities()
{
    Super::OnRep_ActivateAbilities();

    if (!bStartupAbilitiesGiven)
    {
        bStartupAbilitiesGiven = true;
        AbilitiesGivenDelegate.Broadcast();
    }
}

void UAuraAbilitySystemComponent::ClientUpdateAbilityStatus_Implementation(const FGameplayTag &AbilityTag, const FGameplayTag &StatusTag, int32 AbilityLevel)
{
    AbilityStatusChanged.Broadcast(AbilityTag, StatusTag, AbilityLevel);
}

void UAuraAbilitySystemComponent::ClientEffectApplied_Implementation(UAbilitySystemComponent *AbilitySystemComponent, const FGameplayEffectSpec &EffectSpec, FActiveGameplayEffectHandle ActiveEffectHandle)
{
    FGameplayTagContainer TagContainer;
    EffectSpec.GetAllAssetTags(TagContainer);

    EffectAssetTags.Broadcast(TagContainer);
}