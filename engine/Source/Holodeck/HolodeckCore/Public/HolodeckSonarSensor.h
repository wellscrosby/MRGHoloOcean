// MIT License (c) 2021 BYU FRoStLab see LICENSE file

#pragma once

#include "CoreMinimal.h"
#include "HolodeckCore/Public/HolodeckSensor.h"

#include "GenericPlatform/GenericPlatformMath.h"
#include "Octree.h"
#include "Kismet/KismetMathLibrary.h"
#include "Async/ParallelFor.h"

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
	float RangeMax = 3000;

	UPROPERTY(EditAnywhere)
	float InitOctreeRange = 0;

	UPROPERTY(EditAnywhere)
	float RangeMin = 300;

	UPROPERTY(EditAnywhere)
	float Azimuth = 130;

	UPROPERTY(EditAnywhere)
	float Elevation = 20;

	UPROPERTY(EditAnywhere)
	int TicksPerCapture = 1;

	UPROPERTY(EditAnywhere)
	bool ViewRegion = false;

	UPROPERTY(EditAnywhere)
	int ViewOctree = -10;

	UPROPERTY(EditAnywhere)
	float ShadowEpsilon = 0;

	// Call at the beginning of every tick, loads octree
	void initOctree();

	// Finds all the leaves in range
	void findLeaves();

	// Shadow leaves that have been sorted
	void shadowLeaves();

	// Visualizer helpers
	void showBeam(float DeltaTime);
	void showRegion(float DeltaTime);

	// Used to hold leafs when parallelized filtering happens
	TArray<TArray<Octree*>> foundLeaves;

	// Used to hold leafs when parallelized sorting/binning happens
	TArray<TArray<Octree*>> sortedLeaves;

	// Water information (Change to parameter?)
	float density_water = 997;
	float sos_water = 1480;
	float z_water;

	// use for skipping frames
	int TickCounter = 0;

	// various computations we want to cache
	float ATan2Approx(float y, float x);
	float minAzimuth;
	float maxAzimuth;
	float minElev;
	float maxElev;

	bool inRange(Octree* tree);
	void leavesInRange(Octree* tree, TArray<Octree*>& leafs, float stopAt);
	FVector spherToEuc(float r, float theta, float phi, FTransform SensortoWorld);
	
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

	// various computations we want to cache
	float sqrt3_2;
	float sinOffset;
};
