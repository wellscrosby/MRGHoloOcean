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
	// Actual volume, we adjust to compensate for added styrofoam later
	// this->Volume = .01754043;
	this->Volume = .0322;
	// In cm
	this->CenterBuoyancy = FVector(-0.29, -5.96, -1.85); 
	// This can be fudged a bit if we don't want to tilt a little sideways!
	// this->CenterBuoyancy = FVector(-0.46, -5.90, -1.85); 
}

void AHoveringAUV::InitializeAgent() {
	Super::InitializeAgent();
	RootMesh = Cast<UStaticMeshComponent>(RootComponent);
}

// Called every frame
void AHoveringAUV::Tick(float DeltaSeconds) {
	ApplyBuoyantForce();
}