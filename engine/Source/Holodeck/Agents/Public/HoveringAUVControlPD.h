#pragma once

#include "Holodeck.h"

#include "HolodeckPawnController.h"
#include "HoveringAUV.h"
#include "HolodeckControlScheme.h"
#include "SimplePID.h"
#include <math.h>

#include "HoveringAUVControlPD.generated.h"

const float AUV_CONTROL_MAX_LIN_ACCEL = 1;
const float AUV_CONTROL_MAX_ANG_ACCEL = 1;

const float AUV_POS_P = 100;
const float AUV_POS_D = 50;

const float AUV_ROT_P = 0.1;
const float AUV_ROT_D = 0.1;

/**
* UHoveringAUVControlPD
*/
UCLASS()
class HOLODECK_API UHoveringAUVControlPD : public UHolodeckControlScheme {
public:
	GENERATED_BODY()

	UHoveringAUVControlPD(const FObjectInitializer& ObjectInitializer);

	void Execute(void* const CommandArray, void* const InputCommand, float DeltaSeconds) override;

	unsigned int GetControlSchemeSizeInBytes() const override {
		return 6 * sizeof(float);
	}

	void SetController(AHolodeckPawnController* const Controller) { HoveringAUVController = Controller; };

private:
	AHolodeckPawnController* HoveringAUVController;
	AHoveringAUV* HoveringAUV;

	SimplePID PositionController;
	SimplePID RotationController;
};
