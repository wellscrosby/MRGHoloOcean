// MIT License (c) 2021 BYU FRoStLab see LICENSE file

#pragma once

#include "CoreMinimal.h"
#include "HolodeckCore/Public/HolodeckSensor.h"

#include "GenericPlatform/GenericPlatformMath.h"
#include "Octree.h"
#include "Kismet/KismetMathLibrary.h"
#include "Async/ParallelFor.h"
#include "MultivariateNormal.h"

#include "HolodeckSonarSensor.generated.h"

#define Pi 3.1415926535897932384626433832795
/**
 * UHolodeckSonarSensor
 */
UCLASS()
class HOLODECK_API UHolodeckSonarSensor : public UHolodeckSensor
{
	GENERATED_BODY()
	
public:
	/*
	* Default Constructor
	*/
	UHolodeckSonarSensor(){}

	/**
	* Allows parameters to be set dynamically
	*/
	virtual void ParseSensorParms(FString ParmsJson) override;

	/*
	* Cleans up octree
	*/
	virtual void BeginDestroy() override;

protected:
	void TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere)
	float MaxRange = 3000;

	UPROPERTY(EditAnywhere)
	float InitOctreeRange = 0;

	UPROPERTY(EditAnywhere)
	float MinRange = 300;

	UPROPERTY(EditAnywhere)
	float Azimuth = 130;

	UPROPERTY(EditAnywhere)
	float Elevation = 20;

	UPROPERTY(EditAnywhere)
	int TicksPerCapture = 1;

	// Call at the beginning of every tick, loads octree
	void initOctree();

	// Finds all the leaves in range
	void findLeaves();

	// Used to hold leafs when parallelized filtering happens
	TArray<TArray<Octree*>> foundLeaves;

	// Water information (Change to parameter?)
	float density_water = 997;
	float sos_water = 1480;

	// use for skipping frames
	int TickCounter = 0;

	// various computations we want to cache
	float minAzimuth;
	float maxAzimuth;
	float minElev;
	float maxElev;

private:
	/*
	 * Parent
	 * After initialization, Parent contains a pointer to whatever the sensor is attached to.
	 */
	AActor* Parent;

	// holds our implementation of Octrees
	Octree* octree = nullptr;
	TArray<Octree*> agents;
	void viewLeaves(Octree* tree);

	// What octrees we initally make
	TArray<Octree*> toMake;
	// initialize + reserve vectors once
	TArray<Octree*> bigLeaves;

	// for adding noise
	MultivariateNormal<1> aziNoise;
	MultivariateNormal<1> rNoise;

	// various computations we want to cache
	float sqrt2;
	float sinOffset;

	bool inRange(Octree* tree);
	void leavesInRange(Octree* tree, TArray<Octree*>& leafs, float stopAt);
};
