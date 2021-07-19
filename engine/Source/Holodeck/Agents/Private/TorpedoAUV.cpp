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
	this->CenterBuoyancy = FVector(0,0,5); 
	this->CenterMass = FVector(0,0,-5);
	this->MassInKG = 27;
	this->Volume =  MassInKG / WaterDensity; //0.0342867409204;	
}

void ATorpedoAUV::InitializeAgent() {
	RootMesh = Cast<UStaticMeshComponent>(RootComponent);

	Super::InitializeAgent();
}

// Called every frame
void ATorpedoAUV::Tick(float DeltaSeconds) {
	Super::Tick(DeltaSeconds);
	ApplyBuoyantForce();

	// Apply propeller
	float ThrustToApply = FMath::Clamp(CommandArray[0], TAUV_MIN_THRUST, TAUV_MAX_THRUST);
	FVector LocalThrust = FVector(ThrustToApply, 0, 0);
	LocalThrust = ConvertLinearVector(LocalThrust, ClientToUE);
	RootMesh->AddForceAtLocationLocal(LocalThrust, controls[0]);

	// Apply fin forces
	for(int i=1;i<2;i++){
		// ApplyFin(i);
	}
	ApplyFin(1);
	ApplyFin(3);
}

// TODO: Analyze physics and implement more accurate controls here
// These are mostly placeholders while testing
void ATorpedoAUV::ApplyFin(int i){
	// Get rotations
	float commandAngle = CommandArray[1];
	FRotator bodyToWorld = this->GetActorRotation();
	FRotator finToBody = FRotator(commandAngle, 0, 0);

	// get velocity at fin location
	FVector velWorld = RootMesh->GetPhysicsLinearVelocityAtPoint(controls[i]);
	FVector velBody = bodyToWorld.UnrotateVector(velWorld);
	FVector velFin = finToBody.UnrotateVector(velBody);

	// set an upper and lower cap
	// get flow angle
	double angle = UKismetMathLibrary::DegAtan2(-velFin.Z, velFin.X);
	while(angle-commandAngle > 90){
		angle -= 180;
	}
	while(angle-commandAngle < -90){
		angle += 180;
	}

	double u2 = velFin.Z*velFin.Z + velFin.X*velFin.X;
	// TODO: Verify these coefficients
	double angleRad = angle*3.14/180;
	double sin = -FMath::Sin(angle*3.14/180);
	double drag = 0.5 * u2 * sin*sin / 100;
	double lift = 0.5 * u2 * sin / 100;
	
	FVector fW = -FVector(drag, 0, lift);
	FRotator WToBody = FRotator(angle, 0, 0);
	FVector fBody = WToBody.RotateVector(fW);

	if(i == 1){
		UE_LOG(LogHolodeck, Warning, TEXT("pitch: %f, \t sin: %f, \t ca: %f, \t velocity: %s, \t fW: %s"), 
						bodyToWorld.Euler().Y,
						sin, 
						commandAngle,
						*velBody.ToString(), 
						*fW.ToString());
	}

	if(.01 < velBody.Size() && velBody.Size() < 200){
		RootMesh->AddForceAtLocationLocal(fBody, controls[i]);
	}
}