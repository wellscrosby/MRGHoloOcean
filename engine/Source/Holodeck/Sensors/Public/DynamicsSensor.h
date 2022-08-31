// MIT License (c) 2021 BYU FRoStLab see LICENSE file
#pragma once

#include "Holodeck.h"

#include "HolodeckSensor.h"

#include "DynamicsSensor.generated.h"

/**
* DynamicsSensor
* Inherits from the HolodeckSensor class
* Check out the parent class for documentation on all of the overridden functions.
* Reports the acceleration, velocity, position, ang. accel, ang. vel, orientation for use in implementing custom dyammics 
*/
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class HOLODECK_API UDynamicsSensor : public UHolodeckSensor {
	GENERATED_BODY()

public:
	/*
	* Default Constructor
	*/
	UDynamicsSensor();

	/**
	* InitializeSensor
	* Sets up the class
	*/
	virtual void InitializeSensor() override;

	/**
	* Allows parameters to be set dynamically
	*/
	virtual void ParseSensorParms(FString ParmsJson) override;

protected:
	//See HolodeckSensor for the documentation of these overridden functions.
	int GetNumItems() override { return UseRPY ? 18 : 19; };
	int GetItemSize() override { return sizeof(float); };
	void TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere)
	bool UseCOM = true;

	UPROPERTY(EditAnywhere)
	bool UseRPY = true;

private:
	/*
	 * Parent
	 * After initialization, Parent contains a pointer to whatever the sensor is attached to.
	 */
	UPrimitiveComponent* Parent;

	FVector LinearVelocityThen;
	FVector AngularVelocityThen;

	FVector LinearAcceleration;
	FVector LinearVelocity;
	FVector Position;
	FVector AngularAcceleration;
	FVector AngularVelocity;
	FRotator Rotation;
	FQuat Quat;
	FVector RPY;
};
