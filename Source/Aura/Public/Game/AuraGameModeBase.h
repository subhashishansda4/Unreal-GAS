// Copyright Resurrect Studios

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AuraGameModeBase.generated.h"

class UCharacterClassInfo;
class UAbilityInfo;
class UMVVM_LoadSlot;
class USaveGame;
class ULoadScreenSaveGame;

/**
 *
 */
UCLASS()
class AURA_API AAuraGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "Character Class Defaults")
	TObjectPtr<UCharacterClassInfo> CharacterClassInfo;
	
	UPROPERTY(EditDefaultsOnly, Category = "Ability Info")
	TObjectPtr<UAbilityInfo> AbilityInfo;
	
	void SaveSlotData(UMVVM_LoadSlot *LoadSlot, int32 SlotIndex);
	ULoadScreenSaveGame *GetSaveSlotData(const FString &SlotName, int32 SlotIndex) const;
	static void DeleteSlot(const FString &SlotName, int32 SlotIndex);
	ULoadScreenSaveGame *RetrieveInGameSaveData();
	void SaveInGameProgressData(ULoadScreenSaveGame *SaveObject);
	void SaveWorldState(UWorld *World) const;
	void LoadWorldState(UWorld *World) const;

	void TravelToMap(UMVVM_LoadSlot *Slot);
	
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<USaveGame> LoadScreenSaveGameClass;
	
	UPROPERTY(EditDefaultsOnly)
	FString DefaultMapName;
	
	UPROPERTY(EditDefaultsOnly)
	TSoftObjectPtr<UWorld> DefaultMap;
	
	UPROPERTY(EditDefaultsOnly)
	FName DefaultPlayerStartTag;
	
	UPROPERTY(EditDefaultsOnly)
	TMap<FString, TSoftObjectPtr<UWorld>> Maps;
	
	virtual AActor *ChoosePlayerStart_Implementation(AController *Player) override;

protected:
	virtual void BeginPlay() override;
};
