// MIT License (c) 2021 BYU FRoStLab see LICENSE file

#include "Holodeck.h"
#include "ScubaDiverController.h"

AScubaDiverController::AScubaDiverController(const FObjectInitializer& ObjectInitializer)
	: AHolodeckPawnController(ObjectInitializer) {
	UE_LOG(LogTemp, Warning, TEXT("ScubaDiver Controller Initialized"));
}

AScubaDiverController::~AScubaDiverController() {}
