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
	float ThrustToApply = CommandArray[0];
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
	FRotator bodyToWorld = this->GetActorRotation();
	FRotator finToBody = FRotator(CommandArray[1], 0, 0);

	// get velocity at fin location
	FVector velWorld = RootMesh->GetPhysicsLinearVelocityAtPoint(controls[i]);
	FVector velBody = bodyToWorld.UnrotateVector(velWorld);
	FVector velFin = finToBody.UnrotateVector(velBody);

	// get flow angle
	double angle = CommandArray[1]; //UKismetMathLibrary::DegAtan2(-velFin.Z, velFin.X);
	while(angle > 180){
		angle -= 360;
	}
	while(angle < -180){
		angle += 360;
	}

	double u2 = velFin.Z*velFin.Z + velFin.X*velFin.X;
	// TODO: Verify these coefficients
	double angleRad = angle*3.14/180;
	double drag = .5 * u2 * angleRad*angleRad / 100;
	double lift = .5 * u2 * FMath::Abs(angleRad) / 10;

	// TODO: Figure out signs here, they get a bit wonky
	FVector fW = -FVector(drag, 0, lift); //FVector(-FMath::Sign(velFin.X)*drag, 0, -FMath::Sign(velFin.Z)*lift);
	FRotator WToBody = FRotator(angle, 0, 0);
	FVector fBody = WToBody.RotateVector(fW);

	UE_LOG(LogHolodeck, Warning, TEXT("command: %f, w_angle: %f"), CommandArray[1], angle);
	UE_LOG(LogHolodeck, Warning, TEXT("velocity: %s"), *velBody.ToString());
	UE_LOG(LogHolodeck, Warning, TEXT("velocity_fin: %s"), *velFin.ToString());
	UE_LOG(LogHolodeck, Warning, TEXT("fW: %s"), *fW.ToString());
	UE_LOG(LogHolodeck, Warning, TEXT("fBody: %s"), *fBody.ToString());

	RootMesh->AddForceAtLocationLocal(fBody, controls[i]);
}

// void ATorpedoAUV::ApplyFin(int i){
// 	// Get rotations
// 	FRotator bodyToWorld = this->GetActorRotation();
// 	FRotator finToBody = FRotator(CommandArray[1], 0, 0);

// 	// get velocity at fin location
// 	FVector velWorld = RootMesh->GetPhysicsLinearVelocityAtPoint(controls[i]);
// 	FVector velFin = finToBody.UnrotateVector(bodyToWorld.UnrotateVector(velWorld));
// 	// velFin.X = 0;


// 	UE_LOG(LogHolodeck, Warning, TEXT("angle: %f"), angle);
// 	UE_LOG(LogHolodeck, Warning, TEXT("velWorld: %s"), *velWorld.ToString());
// 	UE_LOG(LogHolodeck, Warning, TEXT("u2: %f, du2: %f"), u2, du2);
// 	UE_LOG(LogHolodeck, Warning, TEXT("Lift: %f, Drag: %f"), lift, drag);
// 	// UE_LOG(LogHolodeck, Warning, TEXT("dragDirection: %s"), *dragDirection.ToString());
// 	// UE_LOG(LogHolodeck, Warning, TEXT("liftDirection: %s"), *liftDirection.ToString());
// }