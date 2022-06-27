// MIT License (c) 2019 BYU PCCL see LICENSE file

#pragma once

#include "Holodeck.h"

#include "HolodeckPawnController.h"
#include "TorpedoAUV.h"
#include "TorpedoAUVControlFins.h"

#include "TorpedoAUVController.generated.h"

/**
* A Holodeck Turtle Agent Controller
*/
UCLASS()
class HOLODECK_API ATorpedoAUVController : public AHolodeckPawnController
{
	GENERATED_BODY()

public:
	/**
	* Default Constructor
	*/
	ATorpedoAUVController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	* Default Destructor
	*/
	~ATorpedoAUVController();

	void AddControlSchemes() override {
		// The default controller currently in ControlSchemes index 0 is the dynamics one. We push it back to index 1 with this code.

		// Thruster controller
		UTorpedoAUVControlFins* Thrusters = NewObject<UTorpedoAUVControlFins>();
		Thrusters->SetController(this);
		ControlSchemes.Insert(Thrusters, 0);
	}
};
