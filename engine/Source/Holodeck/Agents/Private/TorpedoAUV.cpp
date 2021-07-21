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
	float ThrustToApply = FMath::Clamp(CommandArray[4], TAUV_MIN_THRUST, TAUV_MAX_THRUST);
	FVector LocalThrust = FVector(ThrustToApply, 0, 0);
	LocalThrust = ConvertLinearVector(LocalThrust, ClientToUE);
	RootMesh->AddForceAtLocationLocal(LocalThrust, thruster);

	// Apply fin forces
	for(int i=0;i<4;i++){
		// ApplyFin(i);
	}
	ApplyFin(1);
	ApplyFin(3);
	// ApplyFin(3);
}

// TODO: Analyze physics and implement more accurate controls here
// These are mostly placeholders while testing
void ATorpedoAUV::ApplyFin(int i){
	// Get rotations
	float commandAngle = CommandArray[i];
	FRotator bodyToWorld = this->GetActorRotation();
	FRotator finToBody = UKismetMathLibrary::ComposeRotators(FRotator(commandAngle, 0, 0), finRotation[i]);

	// get velocity at fin location
	FVector velWorld = RootMesh->GetPhysicsLinearVelocityAtPoint(finTranslation[i]);
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
	double drag = 0.5 * u2 * sin*sin / 1000;
	double lift = 0.5 * u2 * sin / 1000;
	
	FVector fW = -FVector(drag, 0, lift);
	FRotator WToBody = FRotator(angle, 0, 0) + finRotation[i];
	FVector fBody = WToBody.RotateVector(fW);

	if(true){
		UE_LOG(LogHolodeck, Warning, TEXT("fin: %d, pitch: %f, \t w: %f, \t velocity: %s, \t fW: %s"), 
						i,
						bodyToWorld.Euler().Y,
						angle,
						*velBody.ToString(), 
						*fW.ToString());
	}

	RootMesh->AddForceAtLocationLocal(fBody, finTranslation[i]);
	if(.01 < velBody.Size() && velBody.Size() < 200){
	}
	
	FTransform finCoord = FTransform(finToBody, finTranslation[i]) * GetActorTransform();
	DrawDebugCoordinateSystem(GetWorld(), finCoord.GetTranslation(), finCoord.Rotator(), 15, false, .1, ECC_WorldStatic, 1.f);
}