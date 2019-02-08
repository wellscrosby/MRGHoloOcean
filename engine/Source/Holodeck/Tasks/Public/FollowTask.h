#pragma once

#include "Holodeck.h"

#include "TaskSensor.h"

#include "FollowTask.generated.h"

/**
* UFollowTask
* Inherits from the TaskSensor class.
* Calculates follow reward based on distance and line of sight.
*/
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class HOLODECK_API UFollowTask : public UTaskSensor
{
	GENERATED_BODY()

public:
	/**
	* Default Constructor
	*/
	UFollowTask() {}

	/**
	* InitializeSensor
	* Sets up the class
	*/
	virtual void InitializeSensor() override;

	// Actor to follow
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		AActor* ToFollow;

	// Only give reward if target is in sight
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool OnlyWithinSight;

	// Defines the agent's field of view
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float FOVRadians;

	// Defines the minimum distance to recieve positive reward
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float MinDistance;

	// Defines the target's height
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float TargetHeight;
	
protected:
	//Checkout HolodeckSensor.h for the documentation for this overridden function.
	virtual void TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	// Scales score between 0-1 to 0-100
	const int MaxScore = 100;
};
