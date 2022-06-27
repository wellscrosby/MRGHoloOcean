#pragma once

#include "Holodeck.h"

#include "HolodeckPawnController.h"
#include "TorpedoAUV.h"
#include "HolodeckControlScheme.h"
#include <math.h>

#include "TorpedoAUVControlFins.generated.h"

/**
* UTorpedoAUVControlFins
*/
UCLASS()
class HOLODECK_API UTorpedoAUVControlFins : public UHolodeckControlScheme {
public:
	GENERATED_BODY()

	UTorpedoAUVControlFins(const FObjectInitializer& ObjectInitializer);

	void Execute(void* const CommandArray, void* const InputCommand, float DeltaSeconds) override;

	unsigned int GetControlSchemeSizeInBytes() const override {
		return 5 * sizeof(float);
	}

	void SetController(AHolodeckPawnController* const Controller) { TorpedoAUVController = Controller; };

private:
	AHolodeckPawnController* TorpedoAUVController;
	ATorpedoAUV* TorpedoAUV;

};
