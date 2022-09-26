#include "Holodeck.h"
#include "SurfaceVesselControlThrusters.h"


USurfaceVesselControlThrusters::USurfaceVesselControlThrusters(const FObjectInitializer& ObjectInitializer) :
		Super(ObjectInitializer) {}

void USurfaceVesselControlThrusters::Execute(void* const CommandArray, void* const InputCommand, float DeltaSeconds) {
	if (SurfaceVessel == nullptr) {
		SurfaceVessel = static_cast<ASurfaceVessel*>(SurfaceVesselController->GetPawn());
		if (SurfaceVessel == nullptr) {
			UE_LOG(LogHolodeck, Error, TEXT("USurfaceVesselControlThrusters couldn't get SurfaceVessel reference"));
			return;
		}
		
		SurfaceVessel->EnableDamping();
	}

	float* InputCommandFloat = static_cast<float*>(InputCommand);
	float* CommandArrayFloat = static_cast<float*>(CommandArray);

	SurfaceVessel->ApplyBuoyantForce();
	SurfaceVessel->ApplyThrusters(InputCommandFloat);

	// Zero out the physics based controller
	for(int i=0; i<6; i++){
		CommandArrayFloat[i] = 0;
	}
}