// MIT License (c) 2021 BYU FRoStLab see LICENSE file

#pragma once

#include "Holodeck.h"

#include "HolodeckPawnController.h"
#include "HoveringAUVControlThrusters.h"
#include "HoveringAUVControlPD.h"
#include "ScubaDiver.h"

#include "ScubaDiverController.generated.h"

/**
* A Holodeck Turtle Agent Controller
*/
UCLASS()
class HOLODECK_API AScubaDiverController : public AHolodeckPawnController
{
	GENERATED_BODY()

public:
	/**
	* Default Constructor
	*/
	AScubaDiverController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	* Default Destructor
	*/
	~AScubaDiverController();

	void AddControlSchemes() override {
		// The default controller currently in ControlSchemes index 0 is the dynamics one. We push it back to index 2 with this code.

		// Thruster controller
		UHoveringAUVControlThrusters* Thrusters = NewObject<UHoveringAUVControlThrusters>();
		Thrusters->SetController(this);
		ControlSchemes.Insert(Thrusters, 0);

		// Position / orientation controller
		UHoveringAUVControlPD* PD = NewObject<UHoveringAUVControlPD>();
		PD->SetController(this);
		ControlSchemes.Insert(PD, 1);
	}
};
