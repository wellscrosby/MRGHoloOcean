// MIT License (c) 2019 BYU PCCL see LICENSE file

#include "Holodeck.h"
#include "HoveringAUV.h"


// Sets default values
AHoveringAUV::AHoveringAUV() {
	PrimaryActorTick.bCanEverTick = true;

	// Set the defualt controller
	AIControllerClass = LoadClass<AController>(NULL, TEXT("/Script/Holodeck.HoveringAUVController"), NULL, LOAD_None, NULL);
	AutoPossessAI = EAutoPossessAI::PlacedInWorld;

	// Setup buoyancy properties
	this->Volume = 2;
	this->CenterBuoyancy = FVector(0,0,20);
}

void AHoveringAUV::InitializeAgent() {
	Super::InitializeAgent();
	RootMesh = Cast<UStaticMeshComponent>(RootComponent);
}

// Called every frame
void AHoveringAUV::Tick(float DeltaSeconds) {
	ApplyBuoyantForce();
}