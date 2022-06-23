#include "Holodeck.h"
#include "HoveringAUVControlPD.h"


UHoveringAUVControlPD::UHoveringAUVControlPD(const FObjectInitializer& ObjectInitializer) :
		Super(ObjectInitializer) {

	float pos_p = FMath::Sqrt(AUV_POS_WN);
	float pos_d = 2*AUV_POS_ZETA*AUV_POS_WN;
	PositionController = SimplePID(pos_p, 0.0, pos_d);

}

void UHoveringAUVControlPD::Execute(void* const CommandArray, void* const InputCommand, float DeltaSeconds) {
	if (HoveringAUV == nullptr) {
		HoveringAUV = static_cast<AHoveringAUV*>(HoveringAUVController->GetPawn());
		if (HoveringAUV == nullptr) {
			UE_LOG(LogHolodeck, Error, TEXT("UHoveringAUVControlPD couldn't get HoveringAUV reference"));
			return;
		}
		
		HoveringAUV->EnableDamping();
	}

	// Apply gravity & buoyancy
	HoveringAUV->ApplyBuoyantForce();

	float* InputCommandFloat = static_cast<float*>(InputCommand);
	float* CommandArrayFloat = static_cast<float*>(CommandArray);

	// Get desired location
	FVector DesiredPosition = FVector(InputCommandFloat[0], InputCommandFloat[1], InputCommandFloat[2]);

	// Get current COM (frame we're moving) & velocity
	FVector CurrentPosition = HoveringAUV->RootMesh->GetCenterOfMass();
	CurrentPosition = ConvertLinearVector(CurrentPosition, UEToClient);

	FVector CurrentVelocity = HoveringAUV->RootMesh->GetPhysicsLinearVelocity();
	CurrentVelocity = ConvertLinearVector(CurrentVelocity, UEToClient);


	// Compute accelerations to apply
	FVector Accel;
	for(int i=0; i<3; i++){
		Accel[i] = PositionController.ComputePIDDirect(DesiredPosition[i], CurrentPosition[i], CurrentVelocity[i], DeltaSeconds);
		Accel[i] = FMath::Clamp(Accel[i], -AUV_CONTROLL_MAX_LIN_ACCEL, AUV_CONTROLL_MAX_LIN_ACCEL);
	}
	UE_LOG(LogHolodeck, Warning, TEXT("DesPos: %s, CurPos: %s, Accel: %s"), *DesiredPosition.ToString(), *CurrentPosition.ToString(), *Accel.ToString());

	// Fill in with the PD Control
	// Command array is then passed to vehicle as acceleration & angular velocity
	for(int i=0; i<3; i++){
		CommandArrayFloat[i] = Accel[i];
	}
}