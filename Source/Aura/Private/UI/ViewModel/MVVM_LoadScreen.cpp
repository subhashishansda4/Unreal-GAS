// Copyright Resurrect Studios. All Rights Reserved.


#include "UI/ViewModel/MVVM_LoadScreen.h"
#include "Game/AuraGameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "UI/ViewModel/MVVM_LoadSlot.h"
#include "Game/AuraGameInstance.h"

void UMVVM_LoadScreen::InitializeLoadSlots()
{
    LoadSlot_0 = NewObject<UMVVM_LoadSlot>(this, LoadSlotViewModelClass);
    LoadSlot_0->SetLoadSlotName(FString("LoadSlot_0"));
    LoadSlot_0->SlotIndex = 0;
    LoadSlots.Add(0, LoadSlot_0);
    LoadSlot_1 = NewObject<UMVVM_LoadSlot>(this, LoadSlotViewModelClass);
    LoadSlot_1->SetLoadSlotName(FString("LoadSlot_1"));
    LoadSlot_1->SlotIndex = 1;
    LoadSlots.Add(1, LoadSlot_1);
    LoadSlot_2 = NewObject<UMVVM_LoadSlot>(this, LoadSlotViewModelClass);
    LoadSlot_2->SetLoadSlotName(FString("LoadSlot_2"));
    LoadSlot_2->SlotIndex = 2;
    LoadSlots.Add(2, LoadSlot_2);

    SetNumLoadSlots(LoadSlots.Num());
}

UMVVM_LoadSlot *UMVVM_LoadScreen::GetVMLoadSlotByIndex(int32 Index) const
{
    return LoadSlots.FindChecked(Index);
}

void UMVVM_LoadScreen::NewSlotButtonPressed(int32 Slot, const FString &EnteredName)
{
    AAuraGameModeBase *AuraGameMode = Cast<AAuraGameModeBase>(UGameplayStatics::GetGameMode(this));
    
    LoadSlots[Slot]->SetMapName(AuraGameMode->DefaultMapName);
    LoadSlots[Slot]->SetPlayerName(EnteredName);
    LoadSlots[Slot]->SetPlayerLevel(1);
    LoadSlots[Slot]->SlotStatus = Taken;
    LoadSlots[Slot]->PlayerStartTag = AuraGameMode->DefaultPlayerStartTag;

    AuraGameMode->SaveSlotData(LoadSlots[Slot], Slot);
    LoadSlots[Slot]->InitializeSlot();

    UAuraGameInstance *AuraGameInstance = Cast<UAuraGameInstance>(AuraGameMode->GetGameInstance());
    AuraGameInstance->LoadSlotName = LoadSlots[Slot]->LoadSlotName;
    AuraGameInstance->LoadSlotIndex = LoadSlots[Slot]->SlotIndex;
    AuraGameInstance->PlayerStartTag = AuraGameMode->DefaultPlayerStartTag;
}

void UMVVM_LoadScreen::NewGameButtonPressed(int32 Slot)
{
    LoadSlots[Slot]->SetWidgetSwitcherIndex.Broadcast(1);
}

void UMVVM_LoadScreen::SelectSlotButtonPressed(int32 Slot)
{
    SlotSelected.Broadcast();

    for (const TTuple<int32, UMVVM_LoadSlot *> LoadSlot : LoadSlots)
    {
        if (LoadSlot.Key == Slot)
        {
            LoadSlot.Value->EnableSelectSlotButton.Broadcast(false);
        }
        else
        {
            LoadSlot.Value->EnableSelectSlotButton.Broadcast(true);
        }
    }
    SelectedSlot = LoadSlots[Slot];
}

void UMVVM_LoadScreen::DeleteButtonPressed()
{
    if (IsValid(SelectedSlot))
    {
        AAuraGameModeBase::DeleteSlot(SelectedSlot->GetLoadSlotName(), SelectedSlot->SlotIndex);
        SelectedSlot->SlotStatus = Vacant;
        SelectedSlot->InitializeSlot();
        SelectedSlot->EnableSelectSlotButton.Broadcast(true);
    }
}

void UMVVM_LoadScreen::PlayButtonPressed()
{
    AAuraGameModeBase *AuraGameMode = Cast<AAuraGameModeBase>(UGameplayStatics::GetGameMode(this));
    UAuraGameInstance *AuraGameInstance = Cast<UAuraGameInstance>(AuraGameMode->GetGameInstance());
    AuraGameInstance->PlayerStartTag = SelectedSlot->PlayerStartTag;
    AuraGameInstance->LoadSlotName = SelectedSlot->LoadSlotName;
    AuraGameInstance->LoadSlotIndex = SelectedSlot->SlotIndex;

    if (IsValid(SelectedSlot))
    {
        AuraGameMode->TravelToMap(SelectedSlot);
    }
}

void UMVVM_LoadScreen::LoadData()
{
    AAuraGameModeBase *AuraGameMode = Cast<AAuraGameModeBase>(UGameplayStatics::GetGameMode(this));
    for (const TTuple<int32, UMVVM_LoadSlot *> LoadSlot : LoadSlots)
    {
        ULoadScreenSaveGame *SaveObject = AuraGameMode->GetSaveSlotData(LoadSlot.Value->GetLoadSlotName(), LoadSlot.Key);
        
        const FString PlayerName = SaveObject->PlayerName;
        TEnumAsByte<ESaveSlotStatus> SaveSlotStatus = SaveObject->SaveSlotStatus;

        LoadSlot.Value->SlotStatus = SaveSlotStatus;
        LoadSlot.Value->SetPlayerName(PlayerName);
        LoadSlot.Value->InitializeSlot();

        LoadSlot.Value->SetMapName(SaveObject->MapName);
        LoadSlot.Value->PlayerStartTag = SaveObject->PlayerStartTag;
        LoadSlot.Value->SetPlayerLevel(SaveObject->PlayerLevel);
    }
}

void UMVVM_LoadScreen::SetNumLoadSlots(int32 InNumLoadSlots)
{
    UE_MVVM_SET_PROPERTY_VALUE(NumLoadSlots, InNumLoadSlots);
}