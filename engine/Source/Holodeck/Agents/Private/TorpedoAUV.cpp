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
	velFin.X = 0;

	double angle = FMath::Atan2(velFin.Z, velFin.Y);
	double u2 = velFin.Z*velFin.Z + velFin.Y*velFin.Y;
	double du2 = angle * u2;

	double lift = du2;
	double drag = angle * du2;

	FVector liftDirection = -FVector::CrossProduct(FVector(1,0,0), velFin);
	liftDirection.Normalize();
	FVector dragDirection = -velFin;
	dragDirection.Normalize();

	FVector forceFin = lift*liftDirection + drag*dragDirection;
	// FVector forceBody = finToBody.RotateVector(forceFin);

	UE_LOG(LogHolodeck, Warning, TEXT("command: %f"), CommandArray[i]);
	UE_LOG(LogHolodeck, Warning, TEXT("Velocity of Body %d: %s"), i, *velBody.ToString());
	UE_LOG(LogHolodeck, Warning, TEXT("u2: %f, du2: %f"), u2, du2);
	UE_LOG(LogHolodeck, Warning, TEXT("Lift: %f, Drag: %f"), lift, drag);
	UE_LOG(LogHolodeck, Warning, TEXT("forceFin: %s"), *forceFin.ToString());

	RootMesh->AddForce(bodyToWorld.RotateVector(forceFin));
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