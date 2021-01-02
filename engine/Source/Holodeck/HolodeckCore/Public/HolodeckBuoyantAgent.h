// MIT License (c) 2020 BYU PCCL see LICENSE file

#pragma once

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

	const float WaterDensity = 997;
	const float Gravity = 9.81;

	UPROPERTY(BlueprintReadWrite, Category = UAVMesh)
		UStaticMeshComponent* RootMesh;

	float Volume;
	FVector CenterBuoyancy;
	FVector CenterMass;
	float MassInKG;
	
	void ApplyBuoyantForce();
};
