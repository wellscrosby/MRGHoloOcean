#include "Holodeck.h"
#include "SurfaceVesselControlPD.h"


USurfaceVesselControlPD::USurfaceVesselControlPD(const FObjectInitializer& ObjectInitializer) :
		Super(ObjectInitializer), PositionController(SV_POS_P, 0, SV_POS_D), RotationController(SV_ROT_P, 0, SV_ROT_D) { }

void USurfaceVesselControlPD::Execute(void* const CommandArray, void* const InputCommand, float DeltaSeconds) {
	if (SurfaceVessel == nullptr) {
		SurfaceVessel = static_cast<ASurfaceVessel*>(SurfaceVesselController->GetPawn());
		if (SurfaceVessel == nullptr) {
			UE_LOG(LogHolodeck, Error, TEXT("USurfaceVesselControlPD couldn't get SurfaceVessel reference"));
			return;
		}
		
		SurfaceVessel->EnableDamping();
	}

	// Apply gravity & buoyancy
	SurfaceVessel->ApplyBuoyantForce();

	float* InputCommandFloat = static_cast<float*>(InputCommand);
	float* CommandArrayFloat = static_cast<float*>(CommandArray);

	// ALL calculations here are done in HoloOcean frame & units. 
}