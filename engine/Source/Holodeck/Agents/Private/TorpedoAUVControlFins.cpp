#include "Holodeck.h"
#include "TorpedoAUVControlFins.h"


UTorpedoAUVControlFins::UTorpedoAUVControlFins(const FObjectInitializer& ObjectInitializer) :
		Super(ObjectInitializer) {}

void UTorpedoAUVControlFins::Execute(void* const CommandArray, void* const InputCommand, float DeltaSeconds) {
	if (TorpedoAUV == nullptr) {
		TorpedoAUV = static_cast<ATorpedoAUV*>(TorpedoAUVController->GetPawn());
		if (TorpedoAUV == nullptr) {
			UE_LOG(LogHolodeck, Error, TEXT("UTorpedoAUVControlFins couldn't get TorpedoAUV reference"));
			return;
		}
		
		TorpedoAUV->EnableDamping();
	}

	float* InputCommandFloat = static_cast<float*>(InputCommand);
	float* CommandArrayFloat = static_cast<float*>(CommandArray);

	// Buoyancy forces
	TorpedoAUV->ApplyBuoyantForce();

	// Propeller
	TorpedoAUV->ApplyThrust(InputCommandFloat[4]);
	
	// Apply fin forces
	for(int i=0;i<4;i++){
		TorpedoAUV->ApplyFin(i, InputCommandFloat[i]);
	}

	// Zero out the physics based controller
	for(int i=0; i<6; i++){
		CommandArrayFloat[i] = 0;
	}
}