// MIT License (c) 2019 BYU PCCL see LICENSE file

#include "Holodeck.h"
#include "TorpedoAUV.h"

// Sets default values
ATorpedoAUV::ATorpedoAUV() {
	PrimaryActorTick.bCanEverTick = true;

	// Set the default controller
	AIControllerClass = LoadClass<AController>(NULL, TEXT("/Script/Holodeck.TorpedoAUVController"), NULL, LOAD_None, NULL);
	AutoPossessAI = EAutoPossessAI::PlacedInWorld;

	this->OffsetToOrigin = FVector(0, 0, 0);
	this->CenterBuoyancy = FVector(0,0,7); 
	this->CenterMass = FVector(0,0,0);
	this->MassInKG = 36;
	this->Volume =  MassInKG / WaterDensity; //0.0342867409204;

	// These values are completely guesstimations
	this->CoefficientOfDrag = 0.4;
	this->AreaOfDrag = 0.2;
}

void ATorpedoAUV::InitializeAgent() {
	RootMesh = Cast<UStaticMeshComponent>(RootComponent);

	Super::InitializeAgent();
}

// Called every frame
void ATorpedoAUV::Tick(float DeltaSeconds) {
	Super::Tick(DeltaSeconds);

	// Convert linear acceleration to force
	FVector linAccel = FVector(CommandArray[0], CommandArray[1], CommandArray[2]);
	linAccel = ClampVector(linAccel, -FVector(TAUV_MAX_LIN_ACCEL), FVector(TAUV_MAX_LIN_ACCEL));
	linAccel = ConvertLinearVector(linAccel, ClientToUE);

	// Convert angular acceleration to torque
	FVector angAccel = FVector(CommandArray[3], CommandArray[4], CommandArray[5]);
	angAccel = ClampVector(angAccel, -FVector(TAUV_MAX_ANG_ACCEL), FVector(TAUV_MAX_ANG_ACCEL));
	angAccel = ConvertAngularVector(angAccel, NoScale);


	RootMesh->GetBodyInstance()->AddForce(linAccel, true, true);
	RootMesh->GetBodyInstance()->AddTorqueInRadians(angAccel, true, true);
}

/** Based on the models found in
	* Preliminary Evaluation of Cooperative Navigation of Underwater Vehicles 
	* 		without a DVL Utilizing a Dynamic Process Model, Section III-3) Control Inputs
	* 		https://ieeexplore.ieee.org/document/8460970
*/
void ATorpedoAUV::ApplyFin(int i, float command){
	// Get rotations
	float commandAngle = FMath::Clamp(command, TAUV_MIN_FIN, TAUV_MAX_FIN);
	FRotator bodyToWorld = this->GetActorRotation();
	FRotator finToBody = UKismetMathLibrary::ComposeRotators(FRotator(commandAngle, 0, 0), finRotation[i]);

	// get velocity at fin location, in fin frame
	FVector velWorld = RootMesh->GetPhysicsLinearVelocityAtPoint(finTranslation[i]);
	FVector velBody = bodyToWorld.UnrotateVector(velWorld);
	FVector velFin = finToBody.UnrotateVector(velBody);

	// get flow angle and flow frame
	double angle = UKismetMathLibrary::DegAtan2(-velFin.Z, velFin.X);
	while(angle-commandAngle > 90){
		angle -= 180;
	}
	while(angle-commandAngle < -90){
		angle += 180;
	}
	FRotator WToBody = UKismetMathLibrary::ComposeRotators(FRotator(angle, 0, 0), finRotation[i]);

	// Calculate force in flow frame
	double u2 = velFin.Z*velFin.Z + velFin.X*velFin.X;
	// TODO: Verify these coefficients
	// I've just adjusted these until they seem to behave correctly
	double sin = -FMath::Sin(angle*3.14/180);
	double drag = 0.5 * u2 * sin*sin / 400;
	double lift = 0.5 * u2 * sin*sin*sin / 10;
	// Sometimes they get out of hand, clamp them
	FVector fW = -FVector(drag, 0, lift);
	fW = fW.GetClampedToMaxSize(300);
	// flip it if we're going backwards
	if(velBody.X < 0){
		fW *= -1;
	}

	// Move force into body frame & apply
	FVector fBody = WToBody.RotateVector(fW);
	RootMesh->AddForceAtLocationLocal(fBody, finTranslation[i]);

	// // Used to draw forces/frames for debugging
	// UE_LOG(LogHolodeck, Warning, TEXT("Angle: %f, drag %f, lift %f"), angle, fW.X, fW.Z);
	// FTransform finCoord = FTransform(finToBody, finTranslation[i]) * GetActorTransform();
	// FVector fWorld = bodyToWorld.RotateVector(fBody);
	// // View force vector
	// DrawDebugLine(GetWorld(), finCoord.GetTranslation(), finCoord.GetTranslation()+fWorld*10, FColor::Red, false, .1, ECC_WorldStatic, 1.f);
	// // View angle of attack coordinate frame
	// DrawDebugCoordinateSystem(GetWorld(), finCoord.GetTranslation(), finCoord.Rotator(), 15, false, .1, ECC_WorldStatic, 1.f);
}

void ATorpedoAUV::ApplyThrust(float thrust){
	// Apply propeller
	float ThrustToApply = FMath::Clamp(thrust, TAUV_MIN_THRUST, TAUV_MAX_THRUST);
	FVector LocalThrust = FVector(ThrustToApply, 0, 0);
	LocalThrust = ConvertLinearVector(LocalThrust, ClientToUE);
	RootMesh->AddForceAtLocationLocal(LocalThrust, thruster);
}

// For empty dynamics, damping is disabled
// Enable it when using thrusters & fins
void ATorpedoAUV::EnableDamping(){
	// Only damp if we are not doing currents
	if (VecFieldActorPtr == nullptr){
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,"Lin Damping");
		RootMesh->SetLinearDamping(1.0);
	}
	RootMesh->SetAngularDamping(0.5);
}