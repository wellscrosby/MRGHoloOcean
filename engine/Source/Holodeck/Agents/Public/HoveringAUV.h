// MIT License (c) 2021 BYU FRoStLab see LICENSE file

#pragma once

#include "Containers/Array.h"
#include "GameFramework/Pawn.h"
#include "HolodeckBuoyantAgent.h"
#include "HoveringAUV.generated.h"

const float AUV_MAX_LIN_ACCEL = 10;
const float AUV_MAX_ANG_ACCEL = 2;
const float AUV_MAX_THRUST = AUV_MAX_LIN_ACCEL*31.02 / 4;

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

	unsigned int GetRawActionSizeInBytes() const override { return 6 * sizeof(float); };
	void* GetRawActionBuffer() const override { return (void*)CommandArray; };

	// Allows agent to fall up to ~8 meters
	float GetAccelerationLimit() override { return 400; }

	// Location of all thrusters
	TArray<FVector> thrusterLocations{ FVector(18.18, 22.14, -4), 
											FVector(18.18, -22.14, -4),
											FVector(-31.43, -22.14, -4),
											FVector(-31.43, 22.14, -4),
											FVector(7.39, 18.23, -0.21),
											FVector(7.39, -18.23, -0.21),
											FVector(-20.64, -18.23, -0.21),
											FVector(-20.64, 18.23, -0.21) };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BuoyancySettings)
		bool Perfect= true;

	void ApplyThrusters(float* const ThrusterArray);

	void EnableDamping();

private:
	// Accelerations
	float CommandArray[6];

};
