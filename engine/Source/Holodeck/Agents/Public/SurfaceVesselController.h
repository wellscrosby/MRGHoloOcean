// MIT License (c) 2021 BYU FRoStLab see LICENSE file

#pragma once

#include "Holodeck.h"

#include "HolodeckPawnController.h"
#include "SurfaceVesselControlThrusters.h"
#include "SurfaceVesselControlPD.h"
#include "SurfaceVessel.h"

#include "SurfaceVesselController.generated.h"

/**
* A Holodeck Turtle Agent Controller
*/
UCLASS()
class HOLODECK_API ASurfaceVesselController : public AHolodeckPawnController
{
	GENERATED_BODY()

public:
	/**
	* Default Constructor
	*/
	ASurfaceVesselController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	* Default Destructor
	*/
	~ASurfaceVesselController();

	void AddControlSchemes() override {
		// The default controller currently in ControlSchemes index 0 is the dynamics one. We push it back to index 2 with this code.

		// Thruster controller
		USurfaceVesselControlThrusters* Thrusters = NewObject<USurfaceVesselControlThrusters>();
		Thrusters->SetController(this);
		ControlSchemes.Insert(Thrusters, 0);

		// Position / orientation controller
		USurfaceVesselControlPD* PD = NewObject<USurfaceVesselControlPD>();
		PD->SetController(this);
		ControlSchemes.Insert(PD, 1);
	}
};
