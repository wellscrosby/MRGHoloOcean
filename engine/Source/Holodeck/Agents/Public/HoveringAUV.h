// MIT License (c) 2019 BYU PCCL see LICENSE file

#pragma once

#include "GameFramework/Pawn.h"
#include "HolodeckBuoyantAgent.h"
#include "HoveringAUV.generated.h"


UCLASS()
/**
* AHoveringAUV
* Inherits from the HolodeckAgent class
* On any tick this object will apply the given forces.
* Desired values must be set by a controller.
*/
class HOLODECK_API AHoveringAUV : public AHolodeckBuoyantAgent
{
	GENERATED_BODY()

public:
	/**
	* Default Constructor.
	*/
	AHoveringAUV();

	void InitializeAgent() override;

	/**
	* Tick
	* Called each frame.
	* @param DeltaSeconds the time since the last tick.
	*/
	void Tick(float DeltaSeconds) override;

	unsigned int GetRawActionSizeInBytes() const override { return 2 * sizeof(float); };
	void* GetRawActionBuffer() const override { return (void*)CommandArray; };

	// Allows agent to fall up to ~8 meters
	float GetAccelerationLimit() override { return 400; }

private:
	/**
	* 0: ThrustToApply
	* 1: YawTorqueToApply
	*/
	float CommandArray[2];

};
