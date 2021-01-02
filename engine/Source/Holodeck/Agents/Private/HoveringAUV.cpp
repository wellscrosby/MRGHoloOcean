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

	this->Volume = .03554577;	
	this->CenterBuoyancy = FVector(-5.96, 0.29, -1.85); 
	this->CenterMass = FVector(-5.9, 0.46, -2.82);
	this->MassInKG = 31.02;

	// Apply offset to all of our position vectors
	this->CenterBuoyancy += offset;
	this->CenterMass += offset;
	for(int i=0;i<8;i++){
		thrusterLocations[i] += offset;
	}
}

// Sets all values that we need
void AHoveringAUV::InitializeAgent() {
	Super::InitializeAgent();
	RootMesh = Cast<UStaticMeshComponent>(RootComponent);

	if(Perfect){
		this->CenterMass = (thrusterLocations[0] + thrusterLocations[2]) / 2;
		this->CenterMass.Z = thrusterLocations[7].Z;
		
		this->CenterBuoyancy = CenterMass;
		this->CenterBuoyancy.Z += 5;

		this->Volume = MassInKG / WaterDensity;
	}

	// Set COM (have to do some calculation since it sets an offset)
	FVector COM_curr = GetActorRotation().UnrotateVector( RootMesh->GetCenterOfMass() - GetActorLocation() );
	RootMesh->SetCenterOfMass( CenterMass - COM_curr );
	// Set Mass
	RootMesh->SetMassOverrideInKg("", MassInKG);
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

	FVector com = ActorRotation.UnrotateVector( RootMesh->GetCenterOfMass() - ActorLocation) - offset;
	UE_LOG(LogTemp, Warning, TEXT("com, %f %f %f"), com.X, com.Y, com.Z );
	UE_LOG(LogTemp, Warning, TEXT("mass, %f"), RootMesh->GetMass() );

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