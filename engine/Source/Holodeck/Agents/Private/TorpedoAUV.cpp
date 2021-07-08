// MIT License (c) 2019 BYU PCCL see LICENSE file

#include "Holodeck.h"
#include "TorpedoAUV.h"

// Sets default values
ATorpedoAUV::ATorpedoAUV() {
	PrimaryActorTick.bCanEverTick = true;

	// Set the default controller
	AIControllerClass = LoadClass<AController>(NULL, TEXT("/Script/Holodeck.TorpedoAUVController"), NULL, LOAD_None, NULL);
	AutoPossessAI = EAutoPossessAI::PlacedInWorld;

	this->OffsetToOrigin = FVector(0, 0, 0);
	this->CenterBuoyancy = FVector(0,0,5); 
	this->CenterMass = FVector(0,0,-5);
	this->MassInKG = 27;
	this->Volume =  MassInKG / WaterDensity; //0.0342867409204;	
}

void ATorpedoAUV::InitializeAgent() {
	RootMesh = Cast<UStaticMeshComponent>(RootComponent);

	Super::InitializeAgent();
}

// Called every frame
void ATorpedoAUV::Tick(float DeltaSeconds) {
	Super::Tick(DeltaSeconds);
	ApplyBuoyantForce();
	ApplyThrusters();
}

// TODO: Analyze physics and implement more accurate controls here
// These are mostly placeholders while testing
void ATorpedoAUV::ApplyThrusters(){
	float RollTorqueToApply = -CommandArray[0];
	float PitchTorqueToApply = -CommandArray[1];
	float YawTorqueToApply = CommandArray[2];
	float ThrustToApply = CommandArray[3];

	FVector LocalThrust = FVector(ThrustToApply, 0, 0);
	LocalThrust = ConvertLinearVector(LocalThrust, ClientToUE);
	FVector LocalTorque = FVector(RollTorqueToApply, PitchTorqueToApply, YawTorqueToApply);
	LocalTorque = ConvertTorque(LocalTorque, ClientToUE);

	// Apply torques and forces in global coordinates
	RootMesh->AddTorqueInRadians(GetActorRotation().RotateVector(LocalTorque));
	RootMesh->AddForce(GetActorRotation().RotateVector(LocalThrust));
	
}