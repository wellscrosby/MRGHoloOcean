// MIT License (c) 2019 BYU PCCL see LICENSE file

#include "Holodeck.h"
#include "HoveringAUV.h"

const float AUV_MAX_FORCE = 100;

// Sets default values
AHoveringAUV::AHoveringAUV() {
	PrimaryActorTick.bCanEverTick = true;

	// Set the defualt controller
	AIControllerClass = LoadClass<AController>(NULL, TEXT("/Script/Holodeck.HoveringAUVController"), NULL, LOAD_None, NULL);
	AutoPossessAI = EAutoPossessAI::PlacedInWorld;

	// Actual volume without enclosure air
	// this->Volume = .01754043;
	// Actual volume with enclosure air
	this->Volume = .03554577;
	// Fudged volume to make us barely float
	// this->Volume = .0322;
	
	// Actual COB
	this->CenterBuoyancy = FVector(-5.96, 0.29, -1.85); 
	// This can be fudged a bit if we don't want to tilt a little sideways!
	// this->CenterBuoyancy = FVector(5.9, 0.46, -1.85); 

	// Apply offset to all of our position vectors
	this->CenterBuoyancy += offset;
	for(int i=0;i<8;i++){
		thrusterLocations[i] += offset;
	}
}

void AHoveringAUV::InitializeAgent() {
	Super::InitializeAgent();
	RootMesh = Cast<UStaticMeshComponent>(RootComponent);
}

// Called every frame
void AHoveringAUV::Tick(float DeltaSeconds) {
	Super::Tick(DeltaSeconds);
	ApplyBuoyantForce();
	ApplyThrusters();
}

void AHoveringAUV::ApplyThrusters(){
	//Get all the values we need once
    FVector ActorLocation = GetActorLocation();
	FRotator ActorRotation = GetActorRotation();

	FVector com = ActorRotation.UnrotateVector( RootMesh->GetCenterOfMass() - ActorLocation);
	UE_LOG(LogTemp, Warning, TEXT("com, %f %f %f"), com.X, com.Y, com.Z );
	// Iterate through vertical thrusters
	for(int i=0;i<4;i++){
		float force = FMath::Clamp(CommandArray[i], -AUV_MAX_FORCE, AUV_MAX_FORCE);

		FVector LocalForce = FVector(0, 0, force);
		LocalForce = ConvertLinearVector(LocalForce, ClientToUE);

		RootMesh->AddForceAtLocationLocal(LocalForce, thrusterLocations[i]);
	}

	// Iterate through angled thrusters
	for(int i=4;i<8;i++){
		float force = FMath::Clamp(CommandArray[i], -AUV_MAX_FORCE, AUV_MAX_FORCE);

		// 4 + 6 have negative y
		FVector LocalForce = FVector(0, 0, 0);
		if(i % 2 == 0) 	
			LocalForce = FVector(force/FMath::Sqrt(2), -force/FMath::Sqrt(2), 0);
		else	
			LocalForce = FVector(force/FMath::Sqrt(2), force/FMath::Sqrt(2), 0);
		LocalForce = ConvertLinearVector(LocalForce, ClientToUE);

		RootMesh->AddForceAtLocationLocal(LocalForce, thrusterLocations[i]);
	}
	
}