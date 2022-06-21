#include "Holodeck.h"
#include "HoveringAUVControlThrusters.h"


UHoveringAUVControlThrusters::UHoveringAUVControlThrusters(const FObjectInitializer& ObjectInitializer) :
		Super(ObjectInitializer) {}

void UHoveringAUVControlThrusters::Execute(void* const CommandArray, void* const InputCommand, float DeltaSeconds) {
	if (HoveringAUV == nullptr) {
		HoveringAUV = static_cast<AHoveringAUV*>(HoveringAUVController->GetPawn());
		if (HoveringAUV == nullptr) {
			UE_LOG(LogHolodeck, Error, TEXT("UHoveringAUVControlThrusters couldn't get HoveringAUV reference"));
			return;
		}
		
		HoveringAUV->EnableDamping();
	}

	float* InputCommandFloat = static_cast<float*>(InputCommand);
	float* CommandArrayFloat = static_cast<float*>(CommandArray);

	HoveringAUV->ApplyBuoyantForce();
	HoveringAUV->ApplyThrusters(InputCommandFloat);

	// Zero out the physics based controller
	for(int i=0; i<6; i++){
		CommandArrayFloat[i] = 0;
	}
}