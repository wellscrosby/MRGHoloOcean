#pragma once

#include "Holodeck.h"

#include "HolodeckPawnController.h"
#include "SurfaceVessel.h"
#include "HolodeckControlScheme.h"
#include "SimplePID.h"
#include <math.h>

#include "SurfaceVesselControlThrusters.generated.h"

/**
* USurfaceVesselControlThrusters
*/
UCLASS()
class HOLODECK_API USurfaceVesselControlThrusters : public UHolodeckControlScheme {
public:
	GENERATED_BODY()

	USurfaceVesselControlThrusters(const FObjectInitializer& ObjectInitializer);

	void Execute(void* const CommandArray, void* const InputCommand, float DeltaSeconds) override;

	unsigned int GetControlSchemeSizeInBytes() const override {
		return 2 * sizeof(float);
	}

	void SetController(AHolodeckPawnController* const Controller) { SurfaceVesselController = Controller; };

private:
	AHolodeckPawnController* SurfaceVesselController;
	ASurfaceVessel* SurfaceVessel;

};
