// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "Holodeck.h"

#include "HolodeckSensor.h"

#include "VelocitySensor.generated.h"

/**
  * VelocitySensor
  * Inherits from the HolodeckSensor class
  * Check out the parent class for documentation on all of the overridden funcions. 
  * Gets the true velocity of the component that the sensor is attached to. 
  */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class HOLODECK_API UVelocitySensor : public UHolodeckSensor {
	GENERATED_BODY()

public:
	/**
	  * Default Constructor
	  */
	UVelocitySensor();

	/**
	  * BeginPlay
  	  * Called at the start of the game.
	  */
	void BeginPlay() override;

	/**
	  * GetVelocity
	  * Calculates and returns the velocity.
	  * It is dependent on ChangeInTime, LocationAtPreviousTick, and CurrentLocation being updated and current. 
	  * @return the x, y, and z components of instantaneous velocity. 
	  */
	FVector GetVelocity();

protected:
	//See HolodeckSensor for the documentation of these overridden functions.
	FString GetDataKey() override { return "VelocitySensor"; };
	int GetNumItems() override { return 3; };
	int GetItemSize() override { return sizeof(float); };
	void TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	/**
	  * Parent
	  * After initialization, Parent contains a pointer to whatever the sensor is attached to.
	  * Not owned.
	  */
	USceneComponent* Parent;

	//These private member variables are for computing the instantaneous velocity. 
	float ChangeInTime;
	FVector LocationAtPreviousTick;
	FVector CurrentLocation;

};
