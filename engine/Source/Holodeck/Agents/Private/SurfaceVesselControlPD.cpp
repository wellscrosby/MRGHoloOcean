#include "Holodeck.h"
#include "SurfaceVesselControlPD.h"


USurfaceVesselControlPD::USurfaceVesselControlPD(const FObjectInitializer& ObjectInitializer) :
		Super(ObjectInitializer), PositionController(SV_POS_P, 0, SV_POS_D), YawController(SV_YAW_P, 0, SV_YAW_D) {}

void USurfaceVesselControlPD::Execute(void* const CommandArray, void* const InputCommand, float DeltaSeconds) {
	if (SurfaceVessel == nullptr) {
		SurfaceVessel = static_cast<ASurfaceVessel*>(SurfaceVesselController->GetPawn());
		if (SurfaceVessel == nullptr) {
			UE_LOG(LogHolodeck, Error, TEXT("USurfaceVesselControlPD couldn't get SurfaceVessel reference"));
			return;
		}
		
		SurfaceVessel->EnableDamping();
		d = FMath::Abs(SurfaceVessel->thrusterLocations[0].Y) / 100;
	}

	// Apply gravity & buoyancy
	SurfaceVessel->ApplyBuoyantForce();

	float* InputCommandFloat = static_cast<float*>(InputCommand);
	float* CommandArrayFloat = static_cast<float*>(CommandArray);

	// ALL calculations here are done in HoloOcean frame & units. 

	// Get current position, velocity, yaw, and yaw rate
	FVector Position = SurfaceVessel->RootMesh->GetCenterOfMass();
	Position = ConvertLinearVector(Position, UEToClient);
	

	FVector LinearVelocity = SurfaceVessel->RootMesh->GetPhysicsLinearVelocity();
	LinearVelocity = ConvertLinearVector(LinearVelocity, UEToClient);

	float Yaw = RotatorToRPY(SurfaceVessel->GetActorRotation()).Z;

	FVector AngularVelocity = SurfaceVessel->RootMesh->GetPhysicsAngularVelocityInDegrees();
	AngularVelocity = ConvertAngularVector(AngularVelocity, NoScale);
	float YawRate = AngularVelocity.Z;
	
	// Get desired information
	FVector DesiredPosition = FVector(InputCommandFloat[0], InputCommandFloat[1], 0);
	float DesiredYaw = FMath::Atan2(Position.Y-DesiredPosition.Y, Position.X-DesiredPosition.X);
	DesiredYaw = FMath::RadiansToDegrees(DesiredYaw) + 180.0;

	// Compute force & torque to apply
	float tau = YawController.ComputePIDDirect(DesiredYaw, Yaw, YawRate, DeltaSeconds, true, true);
	float f = PositionController.ComputePIDDirect(FVector::Dist2D(DesiredPosition, Position), 0, 0, DeltaSeconds);
	UE_LOG(LogHolodeck, Warning, TEXT("f %f, t %f"), f, tau);

	// Clamp
	tau = FMath::Clamp(tau, -SV_CONTROL_MAX_TORQUE, SV_CONTROL_MAX_TORQUE);
	f = FMath::Clamp(f, -SV_CONTROL_MAX_FORCE, SV_CONTROL_MAX_FORCE);

	// Convert to left & right forces & send to thrusters
	float ThrusterCommands[2];
	ThrusterCommands[0] = f/2 - tau / (2*d);
	ThrusterCommands[1] = f/2 + tau / (2*d);
	SurfaceVessel->ApplyThrusters(ThrusterCommands);

	// Empty out physics controller
	for(int i=0; i<6; i++){
		CommandArrayFloat[i] = 0;
	}
}