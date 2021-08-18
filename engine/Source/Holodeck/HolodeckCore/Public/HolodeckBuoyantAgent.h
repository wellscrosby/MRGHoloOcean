// MIT License (c) 2020 BYU PCCL see LICENSE file

#pragma once

#include "Containers/Array.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/Pawn.h"
#include "HolodeckAgent.h"
#include "HolodeckBuoyantAgent.generated.h"

/**
 * 
 */
UCLASS()
class HOLODECK_API AHolodeckBuoyantAgent : public AHolodeckAgent{
	GENERATED_BODY()
	
public:

	void InitializeAgent() override;

	const float WaterDensity = 997;
	float Gravity;

	UPROPERTY(BlueprintReadWrite, Category = UAVMesh)
		UStaticMeshComponent* RootMesh;

	// Setting up coordinate position
	// This probably should be set
	FVector OffsetToOrigin = FVector(0,0,0);

	// Physical parameters of vehicle
	// These all MUST be set
	float Volume; // in m^3
	FVector CenterBuoyancy; // in cm
	FVector CenterMass; // in cm
	float MassInKG;

	// Used for surface buoyancy.
	// These are optional to set, will be calculated based on mesh 
	FVector CenterVehicle = FVector(0,0,0); // Center of vehicle from origin. NEED to set if origin isn't center of vehicle
	int NumSurfacePoints = 1000;
	FBox BoundingBox = FBox();
	TArray<FVector> SurfacePoints;
	float SurfaceLevel = 0;
	
	void ApplyBuoyantForce();
	void ShowBoundingBox();
	void ShowSurfacePoints();
};
