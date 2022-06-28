#pragma once

#include "Holodeck.h"

#include "HolodeckPawnController.h"
#include "SurfaceVessel.h"
#include "HolodeckControlScheme.h"
#include "SimplePID.h"
#include <math.h>

#include "SurfaceVesselControlPD.generated.h"

const float SV_CONTROL_MAX_LIN_ACCEL = 1;
const float SV_CONTROL_MAX_ANG_ACCEL = 1;

const float SV_POS_P = 100;
const float SV_POS_D = 50;

const float SV_ROT_P = 0.1;
const float SV_ROT_D = 0.1;

/**
* USurfaceVesselControlPD
*/
UCLASS()
class HOLODECK_API USurfaceVesselControlPD : public UHolodeckControlScheme {
public:
	GENERATED_BODY()

	USurfaceVesselControlPD(const FObjectInitializer& ObjectInitializer);

	void Execute(void* const CommandArray, void* const InputCommand, float DeltaSeconds) override;

	unsigned int GetControlSchemeSizeInBytes() const override {
		return 3 * sizeof(float);
	}

	void SetController(AHolodeckPawnController* const Controller) { SurfaceVesselController = Controller; };

private:
	AHolodeckPawnController* SurfaceVesselController;
	ASurfaceVessel* SurfaceVessel;

	SimplePID PositionController;
	SimplePID RotationController;
};
