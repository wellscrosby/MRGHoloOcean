#pragma once

#include "Holodeck.h"

#include "HolodeckPawnController.h"
#include "HoveringAUV.h"
#include "HolodeckControlScheme.h"
#include "SimplePID.h"
#include <math.h>

#include "HoveringAUVControlThrusters.generated.h"

/**
* UHoveringAUVControlThrusters
*/
UCLASS()
class HOLODECK_API UHoveringAUVControlThrusters : public UHolodeckControlScheme {
public:
	GENERATED_BODY()

	UHoveringAUVControlThrusters(const FObjectInitializer& ObjectInitializer);

	void Execute(void* const CommandArray, void* const InputCommand, float DeltaSeconds) override;

	unsigned int GetControlSchemeSizeInBytes() const override {
		return 8 * sizeof(float);
	}

	void SetController(AHolodeckPawnController* const Controller) { HoveringAUVController = Controller; };

private:
	AHolodeckPawnController* HoveringAUVController;
	AHoveringAUV* HoveringAUV;

};
