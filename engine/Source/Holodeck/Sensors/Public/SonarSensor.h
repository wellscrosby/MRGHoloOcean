// MIT License (c) 2019 BYU PCCL see LICENSE file

#pragma once

#include "CoreMinimal.h"
#include "HolodeckCore/Public/HolodeckSensor.h"

#include "GenericPlatform/GenericPlatformMath.h"
#include "Octree.h"
#include "Kismet/KismetMathLibrary.h"
#include "Async/ParallelFor.h"
#include "MultivariateNormal.h"

#include "Json.h"

#include "SonarSensor.generated.h"

#define Pi 3.1415926535897932384626433832795
/**
 * USonarSensor
 */
UCLASS()
class HOLODECK_API USonarSensor : public UHolodeckSensor
{
	GENERATED_BODY()
	
public:
	/*
	* Default Constructor
	*/
	USonarSensor();

	/**
	* InitializeSensor
	* Sets up the class
	*/
	virtual void InitializeSensor() override;

	/**
	* Allows parameters to be set dynamically
	*/
	virtual void ParseSensorParms(FString ParmsJson) override;

	/*
	* Cleans up octree
	*/
	virtual void BeginDestroy() override;

protected:
	//See HolodeckSensor for the documentation of these overridden functions.
	int GetNumItems() override { return BinsRange*BinsAzimuth; };
	int GetItemSize() override { return sizeof(float); };
	void TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere)
	float MaxRange = 3000;

	UPROPERTY(EditAnywhere)
	float MinRange = 300;

	UPROPERTY(EditAnywhere)
	float Azimuth = 130;

	UPROPERTY(EditAnywhere)
	float Elevation = 20;

	UPROPERTY(EditAnywhere)
	int BinsRange = 300;

	UPROPERTY(EditAnywhere)
	int BinsAzimuth = 128;

	UPROPERTY(EditAnywhere)
	bool ViewRegion = false;

	UPROPERTY(EditAnywhere)
	bool ViewOctree = false;

	UPROPERTY(EditAnywhere)
	int TicksPerCapture = 1;

private:
	/*
	 * Parent
	 * After initialization, Parent contains a pointer to whatever the sensor is attached to.
	 */
	AActor* Parent;

	// holds our implementation of Octrees
	TArray<Octree*>& getOctree(){ return Controller->GetServer()->octree; }
	void viewLeafs(Octree* tree);
	void initOctree();

	// various computations we want to cache
	float RangeRes;
	float AzimuthRes;
	float minAzimuth;
	float maxAzimuth;
	float minElev;
	float maxElev;
	float sinOffset;
	float sqrt2;
	float OctreeMax;

	// initialize + reserve vectors once
	TArray<Octree*> leafs;
	// Used to hold leafs when parallelized filtering happens
	TArray<TArray<Octree*>> tempLeafs;
	// Used to hold leafs when parallelized sorting happens
	TArray<TArray<Octree*>> sortedLeafs;
	int32* count;
	
	// for adding noise
	MultivariateNormal<2> addNoise;
	MultivariateNormal<1> multNoise;

	// use for skipping frames
	int TickCounter = 0;

	bool inRange(Octree* tree, float size);
	void leafsInRange(Octree* tree, TArray<Octree*>& leafs, float size);
};
