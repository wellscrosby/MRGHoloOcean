// MIT License (c) 2021 BYU FRoStLab see LICENSE file

#include "Holodeck.h"
#include "SurfaceVessel.h"

// Sets default values
ASurfaceVessel::ASurfaceVessel() {
	PrimaryActorTick.bCanEverTick = true;

	// Set the defualt controller
	AIControllerClass = LoadClass<AController>(NULL, TEXT("/Script/Holodeck.SurfaceVesselController"), NULL, LOAD_None, NULL);
	AutoPossessAI = EAutoPossessAI::PlacedInWorld;

	// This values are all pulled from the solidworks file
	this->CenterBuoyancy = FVector(0, 0, 10); 
	this->CenterMass = FVector(0, 0, 0);
	this->MassInKG = 200;
	this->OffsetToOrigin = FVector(0, 0, 20);
	this->Volume = 6 * MassInKG / WaterDensity;	
	
	this->BoundingBox = FBox(FVector(-250, -120, -25), FVector(250, 120, 25));

	// Shift thruster locations by offset to the origin
	for(int i=0;i<2;i++){
		thrusterLocations[i] += this->OffsetToOrigin;
	}
}

// Sets all values that we need
void ASurfaceVessel::InitializeAgent() {
	RootMesh = Cast<UStaticMeshComponent>(RootComponent);

	Super::InitializeAgent();
}

// Called every frame
void ASurfaceVessel::Tick(float DeltaSeconds) {
	Super::Tick(DeltaSeconds);

	// Convert linear acceleration to force
	FVector linAccel = FVector(CommandArray[0], CommandArray[1], CommandArray[2]);
	linAccel = ClampVector(linAccel, -FVector(SV_MAX_LIN_ACCEL), FVector(SV_MAX_LIN_ACCEL));
	linAccel = ConvertLinearVector(linAccel, ClientToUE);

	// Convert angular acceleration to torque
	FVector angAccel = FVector(CommandArray[3], CommandArray[4], CommandArray[5]);
	angAccel = ClampVector(angAccel, -FVector(SV_MAX_ANG_ACCEL), FVector(SV_MAX_ANG_ACCEL));
	angAccel = ConvertAngularVector(angAccel, NoScale);


	RootMesh->GetBodyInstance()->AddForce(linAccel, true, true);
	RootMesh->GetBodyInstance()->AddTorqueInRadians(angAccel, true, true);
}

// For empty dynamics, damping is disabled
// Enable it when using thrusters/controller
void ASurfaceVessel::EnableDamping(){
	RootMesh->SetLinearDamping(3);
	RootMesh->SetAngularDamping(0.75);
}

void ASurfaceVessel::ApplyThrusters(float* const ThrusterArray){
	// Iterate through 2 thrusters
	for(int i=0;i<2;i++){
		float force = FMath::Clamp(ThrusterArray[i], -SV_MAX_THRUST, SV_MAX_THRUST);

		FVector LocalForce = FVector(force, 0, 0);
		LocalForce = ConvertLinearVector(LocalForce, ClientToUE);

		RootMesh->AddForceAtLocationLocal(LocalForce, thrusterLocations[i]);
	}
}