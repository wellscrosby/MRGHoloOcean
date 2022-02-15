// MIT License (c) 2021 BYU FRoStLab see LICENSE file

#pragma once

#include "CoreMinimal.h"
#include "HolodeckCore/Public/HolodeckSonarSensor.h"

#include "GenericPlatform/GenericPlatformMath.h"
#include "Octree.h"
#include "Kismet/KismetMathLibrary.h"
#include "Async/ParallelFor.h"
#include "MultivariateNormal.h"

#include "Json.h"

#include "SidescanSonarSensor.generated.h"

#define Pi 3.1415926535897932384626433832795
/**
 * USidescanSonarSensor
 */
UCLASS()
class HOLODECK_API USidescanSonarSensor : public UHolodeckSonarSensor
{
    GENERATED_BODY()

public:
    /*
    * Default Constructor
    */
   USidescanSonarSensor();

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
	// int GetNumItems() override { return BinsRange*BinsAzimuth; }; // Returns 2D array for Buffer for Imaging Sonar
	int GetNumItems() override { return BinsRange; }; // Returns 1D array for buffer for Sidescan Sonar
	int GetItemSize() override { return sizeof(float); };
	void TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere)
	int32 BinsRange = 300;

	UPROPERTY(EditAnywhere)
	int32 BinsAzimuth = 600;

	// Elevation bins are used for shadowing, but ultimately no elevation information is retained in the output
	UPROPERTY(EditAnywhere)
	int32 BinsElevation = 0;

	UPROPERTY(EditAnywhere)
	bool ViewRegion = false;

	UPROPERTY(EditAnywhere)
	int ViewOctree = -10;

private:
	/*
	 * Parent
	 * After initialization, Parent contains a pointer to whatever the sensor is attached to.
	 */
	AActor* Parent;

	// various computations we want to cache
	float RangeRes;
	float AzimuthRes;
	float ElevRes;

	// Used to hold leafs when parallelized sorting/binning happens
	TArray<TArray<Octree*>> sortedLeaves;
	int32* count;
	
	// for adding noise
	MultivariateNormal<1> addNoise;
	MultivariateNormal<1> multNoise;

	float density_water = 997;
	float sos_water = 1480;
};
