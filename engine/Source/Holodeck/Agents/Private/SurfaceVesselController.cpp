// MIT License (c) 2021 BYU FRoStLab see LICENSE file

#include "Holodeck.h"
#include "SurfaceVesselController.h"

ASurfaceVesselController::ASurfaceVesselController(const FObjectInitializer& ObjectInitializer)
	: AHolodeckPawnController(ObjectInitializer) {
	UE_LOG(LogTemp, Warning, TEXT("SurfaceVessel Controller Initialized"));
}

ASurfaceVesselController::~ASurfaceVesselController() {}
