// Copyright Resurrect Studios. All Rights Reserved.


#include "Checkpoint/Checkpoint.h"
#include "Components/SphereComponent.h"
#include "Interaction/PlayerInterface.h"
#include "Game/AuraGameModeBase.h"
#include "Kismet/GameplayStatics.h"

ACheckpoint::ACheckpoint(const FObjectInitializer &ObjectInitializer) : Super(ObjectInitializer)
{
    PrimaryActorTick.bCanEverTick = false;

    CheckpointMesh = CreateDefaultSubobject<UStaticMeshComponent>("Checkpoint Mesh");
    CheckpointMesh->SetupAttachment(GetRootComponent());
    CheckpointMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    CheckpointMesh->SetCollisionResponseToAllChannels(ECR_Block);

    CheckpointMesh->SetCustomDepthStencilValue(CustomDepthStencilOverride);
    CheckpointMesh->MarkRenderStateDirty();

    Sphere = CreateDefaultSubobject<USphereComponent>("Sphere");
    Sphere->SetupAttachment(CheckpointMesh);
    Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    Sphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

    MoveToComponent = CreateDefaultSubobject<USceneComponent>("MoveTo Component");
    MoveToComponent->SetupAttachment(GetRootComponent());
}

void ACheckpoint::LoadActor_Implementation()
{
    if (bReached)
    {
        HandleGlowEffects();
    }
}

void ACheckpoint::OnSphereOverlap(UPrimitiveComponent *OverlappedComponent, AActor *OtherActor, UPrimitiveComponent *OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult)
{
    if (OtherActor->Implements<UPlayerInterface>())
    {
        bReached = true;
        if (AAuraGameModeBase *AuraGM = Cast<AAuraGameModeBase>(UGameplayStatics::GetGameMode(this)))
        {
            AuraGM->SaveWorldState(GetWorld());
        }
        IPlayerInterface::Execute_SaveProgress(OtherActor, PlayerStartTag);
        HandleGlowEffects();
        
    }
}

void ACheckpoint::BeginPlay()
{
    Super::BeginPlay();

    Sphere->OnComponentBeginOverlap.AddDynamic(this, &ACheckpoint::OnSphereOverlap);
}

void ACheckpoint::SetMoveToLocation_Implementation(FVector &OutDestination)
{
    OutDestination = MoveToComponent->GetComponentLocation();
}

void ACheckpoint::HighlightActor_Implementation()
{
    CheckpointMesh->SetRenderCustomDepth(true);
}

void ACheckpoint::UnHighlightActor_Implementation()
{
    CheckpointMesh->SetRenderCustomDepth(false);
}

void ACheckpoint::HandleGlowEffects()
{
    Sphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    UMaterialInstanceDynamic *DynamicMaterialInstance = UMaterialInstanceDynamic::Create(CheckpointMesh->GetMaterial(0), this);
    CheckpointMesh->SetMaterial(0, DynamicMaterialInstance);
    CheckpointReached(DynamicMaterialInstance);
}