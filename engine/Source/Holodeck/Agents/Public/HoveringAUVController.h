// MIT License (c) 2021 BYU FRoStLab see LICENSE file

#pragma once

#include "Holodeck.h"

#include "HolodeckPawnController.h"
#include "HoveringAUVControlThrusters.h"
#include "HoveringAUV.h"

#include "HoveringAUVController.generated.h"

/**
* A Holodeck Turtle Agent Controller
*/
UCLASS()
class HOLODECK_API AHoveringAUVController : public AHolodeckPawnController
{
	GENERATED_BODY()

public:
	/**
	* Default Constructor
	*/
	AHoveringAUVController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	* Default Destructor
	*/
	~AHoveringAUVController();

	void AddControlSchemes() override {
		// Thruster controller
		UHoveringAUVControlThrusters* Thrusters = NewObject<UHoveringAUVControlThrusters>();
		Thrusters->SetController(this);
		ControlSchemes.Insert(Thrusters, 0);

		// Position / orientation controller
	}
};
