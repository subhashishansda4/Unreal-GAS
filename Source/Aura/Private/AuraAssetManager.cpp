// Copyright Resurrect Studios


#include "AuraAssetManager.h"
#include "AuraGameplayTags.h"
#include "AbilitySystemGlobals.h"

UAuraAssetManager &UAuraAssetManager::Get()
{
    check(GEngine);
    UAuraAssetManager *AuraAssetManager = Cast<UAuraAssetManager>(GEngine->AssetManager);
    return *AuraAssetManager;
}

void UAuraAssetManager::StartInitialLoading()
{
    Super::StartInitialLoading();

    FAuraGameplayTags::InitializeNativeGameplayTags();
    // !Required to use Target data
    UAbilitySystemGlobals::Get().InitGlobalData();
}