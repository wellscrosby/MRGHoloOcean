// MIT License (c) 2020 BYU PCCL see LICENSE file

#include "Holodeck.h"
#include "HolodeckBuoyantAgent.h"

void AHolodeckBuoyantAgent::InitializeAgent(){
	Super::InitializeAgent();

	// Set COM (have to do some calculation since it sets an offset)
	FVector COM_curr = GetActorRotation().UnrotateVector( RootMesh->GetCenterOfMass() - GetActorLocation() );
	RootMesh->SetCenterOfMass( CenterMass - COM_curr );
	// Set Mass
	RootMesh->SetMassOverrideInKg("", MassInKG);

	// Set Bounding Box (if it hasn't been set by hand)
	if(BoundingBox.GetExtent() == FVector(0, 0, 0))
		BoundingBox = RootMesh->GetStaticMesh()->GetBoundingBox();

	// Sample points (if they haven't already been set)
	if(SurfacePoints.Num() == 0){
		for(int i=0;i<NumSurfacePoints;i++){
			FVector random = UKismetMathLibrary::RandomPointInBoundingBox(FVector(0,0,0), BoundingBox.GetExtent());
			SurfacePoints.Add( random + OffsetToOrigin + CenterVehicle );
		}
	}
	// Otherwise make sure our count is correct (we'll use it later)
	else{
		NumSurfacePoints = SurfacePoints.Num();
	}
}

void AHolodeckBuoyantAgent::ApplyBuoyantForce(){
    //Get all the values we need once
    FVector ActorLocation = GetActorLocation();
	FRotator ActorRotation = GetActorRotation();

	// Check to see how underwater we are
	FVector* points = SurfacePoints.GetData();
	int count = 0;
	for(int i=0;i<NumSurfacePoints;i++){	
		FVector p_world = ActorLocation + ActorRotation.RotateVector(points[i]);
		if(p_world.Z < SurfaceLevel)
			count++;
	}
	float ratio = count*1.0 / NumSurfacePoints;

    // Get and apply Buoyant Force
	float BuoyantForce = Volume * Gravity * WaterDensity * ratio;
	FVector BuoyantVector = FVector(0, 0, BuoyantForce);
	BuoyantVector = ConvertLinearVector(BuoyantVector, ClientToUE);

    FVector COB_World = ActorLocation + ActorRotation.RotateVector(CenterBuoyancy);
	RootMesh->AddForceAtLocation(BuoyantVector, COB_World);
}

void AHolodeckBuoyantAgent::ShowBoundingBox(){
	FVector location = GetActorLocation() + GetActorRotation().RotateVector(OffsetToOrigin + CenterVehicle);
	DrawDebugBox(GetWorld(), location, BoundingBox.GetExtent(), GetActorQuat(), FColor::Red, false, 0.05, 0, 1);
}

void AHolodeckBuoyantAgent::ShowSurfacePoints(){
	FVector ActorLocation = GetActorLocation();
	FRotator ActorRotation = GetActorRotation();
	FVector* points = SurfacePoints.GetData();

	for(int i=0;i<NumSurfacePoints;i++){
		FVector p_world = ActorLocation + ActorRotation.RotateVector(points[i]);
		DrawDebugPoint(GetWorld(), p_world, 5, FColor::Red, false, 0.05);
	}
}