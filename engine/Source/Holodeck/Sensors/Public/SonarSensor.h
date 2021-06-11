// MIT License (c) 2019 BYU PCCL see LICENSE file

#pragma once

#include "CoreMinimal.h"
#include "HolodeckCore/Public/HolodeckSensor.h"

#include "GenericPlatform/GenericPlatformMath.h"
#include "Octree.h"
#include "Kismet/KismetMathLibrary.h"

#include "Json.h"
#include "HAL/FileManagerGeneric.h"

#include "SonarSensor.generated.h"

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
	int MaxRange = 3000;

	UPROPERTY(EditAnywhere)
	int MinRange = 300;

	UPROPERTY(EditAnywhere)
	int Azimuth = 130;

	UPROPERTY(EditAnywhere)
	int Elevation = 20;

	UPROPERTY(EditAnywhere)
	int BinsRange = 300;

	UPROPERTY(EditAnywhere)
	int BinsAzimuth = 128;

	UPROPERTY(EditAnywhere)
	float OctreeMax = 256;

	UPROPERTY(EditAnywhere)
	float OctreeMin = 8;

	UPROPERTY(EditAnywhere)
	bool ViewDebug = false;

private:
	/*
	 * Parent
	 * After initialization, Parent contains a pointer to whatever the sensor is attached to.
	 */
	AActor* Parent;

	// holds our implementation of Octrees
	static TArray<Octree*> octree;
	static FVector EnvMin;
	static FVector EnvMax;
	float RangeRes;
	float AzimuthRes;

	float minAzimuth;
	float maxAzimuth;
	float minElev;
	float maxElev;

	bool inRange(Octree* tree);
	void leafsInRange(Octree* tree, TArray<Octree*>& leafs);
};
