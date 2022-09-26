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

	/** NOTE: These go counter-clockwise, starting in front right
	* 0: Vertical Front Starboard Thruster
	* 1: Vertical Front Port Thruster
	* 2: Vertical Back  Port Thruster
	* 3: Vertical Back  Starboard Thruster
	* 4: Angled   Front Starboard Thruster
	* 5: Angled   Front Port Thruster
	* 6: Angled   Back  Port Thruster
	* 7: Angled   Back  Starboard Thruster
	*/
	unsigned int GetControlSchemeSizeInBytes() const override {
		return 8 * sizeof(float);
	}

	void SetController(AHolodeckPawnController* const Controller) { HoveringAUVController = Controller; };

private:
	AHolodeckPawnController* HoveringAUVController;
	AHoveringAUV* HoveringAUV;

};
