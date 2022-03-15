// MIT License (c) 2021 BYU FRoStLab see LICENSE file

#pragma once

#include "Holodeck.h"
#include "HolodeckSensor.h"

#include "ImagingSonarSensor.h"

#include "ProfilingSonarSensor.generated.h"

/**
 * UProfilingSonarSensor
 */
UCLASS()
class HOLODECK_API UProfilingSonarSensor : public UImagingSonarSensor
{
	GENERATED_BODY()
	
public:
	/*
	* Default Constructor
	*/
	UProfilingSonarSensor();

	/**
	* Allows parameters to be set dynamically
	*/
	virtual void ParseSensorParms(FString ParmsJson) override;

};
