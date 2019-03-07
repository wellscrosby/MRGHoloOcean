#pragma once

#include "Holodeck.h"

#include "TaskSensor.h"

#include "DistanceTask.generated.h"

/**
* UDistanceTask
* Inherits from the TaskSensor class.
* Calculates a distance based reward.
* If UseDistanceReward is true, the reward is the change in distance to the goal.
* If UseDistanceReward is false, the reward is 1 if the agent reaches its next distance goal
* defined by the interval and -1 otherwise. 
* Terminal is set to true when the agent is within its GoalDistance.
*/
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class HOLODECK_API UDistanceTask : public UTaskSensor
{
	GENERATED_BODY()

public:
	/**
	* Default Constructor
	*/
	UDistanceTask() {}

	/**
	* InitializeSensor
	* Sets up the class
	*/
	virtual void InitializeSensor() override;

	// Distance to next reward (if not UseDistanceReward)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float Interval;

	// Required proximity for terminal
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float GoalDistance;

	// True: Reward is based on distance, False: Reward is scarce and +-1
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		bool UseDistanceReward;

	// Goal
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		AActor* GoalObject;

protected:
	//Checkout HolodeckSensor.h for the documentation for this overridden function.
	virtual void TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	/**
	* SetUnitReward
	* Sets the reward if UseDistanceReward is false.
	*/
	void SetUnitReward();

	/**
	* SetDistanceReward
	* Sets the reward if UseDistanceReward is true.
	*/
	void SetDistanceReward();

	float NextDistance;
	float StartDistance;
	float LastDistance;
};
