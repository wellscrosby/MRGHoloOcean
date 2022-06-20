// MIT License (c) 2021 BYU FRoStLab see LICENSE file

#include "Holodeck.h"
#include "HoveringAUV.h"

const float AUV_MAX_FORCE = 100;

// Sets default values
AHoveringAUV::AHoveringAUV() {
	PrimaryActorTick.bCanEverTick = true;

	// Set the defualt controller
	AIControllerClass = LoadClass<AController>(NULL, TEXT("/Script/Holodeck.HoveringAUVController"), NULL, LOAD_None, NULL);
	AutoPossessAI = EAutoPossessAI::PlacedInWorld;

	// This values are all pulled from the solidworks file
	this->Volume = .03554577;	
	this->CenterBuoyancy = FVector(-5.96, 0.29, -1.85); 
	this->CenterMass = FVector(-5.9, 0.46, -2.82);
	this->MassInKG = 31.02;
	this->OffsetToOrigin = FVector(-0.7, -2, 32);
}

// Sets all values that we need
void AHoveringAUV::InitializeAgent() {
	RootMesh = Cast<UStaticMeshComponent>(RootComponent);

	if(Perfect){
		this->CenterMass = (thrusterLocations[0] + thrusterLocations[2]) / 2;
		this->CenterMass.Z = thrusterLocations[7].Z;
		
		this->CenterBuoyancy = CenterMass;
		this->CenterBuoyancy.Z += 5;

		this->Volume = MassInKG / WaterDensity;
	}

	// Apply OffsetToOrigin to all of our custom position vectors
	for(int i=0;i<8;i++){
		thrusterLocations[i] += OffsetToOrigin;
	}

	Super::InitializeAgent();
}

// Called every frame
void AHoveringAUV::Tick(float DeltaSeconds) {
	Super::Tick(DeltaSeconds);

	// Convert linear acceleration to force
	FVector lin_acceleration = FVector(CommandArray[0], CommandArray[1], CommandArray[2]);
	lin_acceleration = ConvertLinearVector(lin_acceleration, ClientToUE);

	FVector force = lin_acceleration*this->MassInKG;
	force = ClampVector(force, -FVector(AUV_MAX_FORCE), FVector(AUV_MAX_FORCE));


	// Convert angular acceleration to torque
	FVector ang_acceleration = FVector(CommandArray[3], CommandArray[4], CommandArray[5]);
	ang_acceleration = ConvertTorque(ang_acceleration, ClientToUE);

	FVector torque = RootMesh->ScaleByMomentOfInertia(ang_acceleration);
	torque = ClampVector(torque, -FVector(AUV_MAX_FORCE), FVector(AUV_MAX_FORCE));

	RootMesh->AddForce(force);
	RootMesh->AddTorqueInRadians(torque);
}

void AHoveringAUV::EnableDamping(){
	RootMesh->SetLinearDamping(1.0);
	RootMesh->SetAngularDamping(0.75);
}

void AHoveringAUV::ApplyThrusters(float* const ThrusterArray){
	//Get all the values we need once
    FVector ActorLocation = GetActorLocation();
	FRotator ActorRotation = GetActorRotation();

	// Iterate through vertical thrusters
	for(int i=0;i<4;i++){
		float force = FMath::Clamp(ThrusterArray[i], -AUV_MAX_FORCE, AUV_MAX_FORCE);

		FVector LocalForce = FVector(0, 0, force);
		LocalForce = ConvertLinearVector(LocalForce, ClientToUE);

		RootMesh->AddForceAtLocationLocal(LocalForce, thrusterLocations[i]);
	}

	// Iterate through angled thrusters
	for(int i=4;i<8;i++){
		float force = FMath::Clamp(ThrusterArray[i], -AUV_MAX_FORCE, AUV_MAX_FORCE);

		// 4 + 6 have negative y
		FVector LocalForce = FVector(0, 0, 0);
		if(i % 2 == 0) 	
			LocalForce = FVector(force/FMath::Sqrt(2), force/FMath::Sqrt(2), 0);
		else	
			LocalForce = FVector(force/FMath::Sqrt(2), -force/FMath::Sqrt(2), 0);
		LocalForce = ConvertLinearVector(LocalForce, ClientToUE);

		RootMesh->AddForceAtLocationLocal(LocalForce, thrusterLocations[i]);
	}

}