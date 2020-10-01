// MIT License (c) 2020 BYU PCCL see LICENSE file

#include "Holodeck.h"
#include "HolodeckBuoyantAgent.h"


void AHolodeckBuoyantAgent::ApplyBuoyantForce(){
    //Get all the values we need once
    FVector ActorLocation = GetActorLocation();
	FRotator ActorRotation = GetActorRotation();

    // Get and apply Buoyant Force
	float BuoyantForce = Volume * Gravity * WaterDensity;
	FVector BuoyantVector = FVector(0, 0, BuoyantForce);
	BuoyantVector = ConvertLinearVector(BuoyantVector, ClientToUE);

    FVector COB_World = ActorLocation + ActorRotation.RotateVector(CenterBuoyancy);
	RootMesh->AddForceAtLocation(BuoyantVector, COB_World);
}
