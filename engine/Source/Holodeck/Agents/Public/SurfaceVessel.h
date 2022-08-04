// MIT License (c) 2021 BYU FRoStLab see LICENSE file

#pragma once

#include "Containers/Array.h"
#include "GameFramework/Pawn.h"
#include "HolodeckBuoyantAgent.h"
#include "SurfaceVessel.generated.h"

const float SV_MAX_LIN_ACCEL = 20;
const float SV_MAX_ANG_ACCEL = 2;
const float SV_MAX_THRUST = 1500;

UCLASS()
/**
* ASurfaceVessel
* Inherits from the HolodeckAgent class
* On any tick this object will apply the given forces.
* Desired values must be set by a controller.
*/
class HOLODECK_API ASurfaceVessel : public AHolodeckBuoyantAgent
{
	GENERATED_BODY()

public:
	/**
	* Default Constructor.
	*/
	ASurfaceVessel();

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

	// Location of all thrusters - Left and Right
	TArray<FVector> thrusterLocations{ FVector(-250, -100, 0), FVector(-250, 100, 0) };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BuoyancySettings)
		bool Perfect= true;

	void ApplyThrusters(float* const ThrusterArray);

	void EnableDamping();

private:
	float CommandArray[6];

};
