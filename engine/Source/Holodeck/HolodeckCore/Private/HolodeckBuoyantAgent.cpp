// MIT License (c) 2021 BYU FRoStLab see LICENSE file

#include "Holodeck.h"
#include "HolodeckBuoyantAgent.h"
#include "VectorField/VectorFieldStatic.h"

void AHolodeckBuoyantAgent::InitializeAgent(){
	Super::InitializeAgent();

	// Get GravityVectority from world
	AWorldSettings* WorldSettings = GetWorld()->GetWorldSettings(false, false);
	Gravity = WorldSettings->GetGravityZ() / -100;

	// Set Mass
	RootMesh->SetMassOverrideInKg("", MassInKG);
	// Set COM (have to do some calculation since it sets an offset)
	FVector COM_curr = GetActorRotation().UnrotateVector( RootMesh->GetCenterOfMass() - GetActorLocation() );
	RootMesh->SetCenterOfMass( CenterMass + OffsetToOrigin - COM_curr );

	// Set Bounding Box (if it hasn't been set by hand)
	if(BoundingBox.GetExtent() == FVector(0, 0, 0))
		BoundingBox = RootMesh->GetStaticMesh()->GetBoundingBox();

	// Sample points (if they haven't already been set)
	if(SurfacePoints.Num() == 0){
		for(int i=0;i<NumSurfacePoints;i++){
			FVector random = UKismetMathLibrary::RandomPointInBoundingBox(FVector(0,0,0), BoundingBox.GetExtent());
			// We pre-add all offsets to reduce computation during simulation
			SurfacePoints.Add( random + OffsetToOrigin + CenterVehicle );
		}
	}
	// Otherwise make sure our count is correct (we'll use it later)
	else{
		NumSurfacePoints = SurfacePoints.Num();
	}

	VecFieldActorPtr = UGameplayStatics::GetActorOfClass(GetWorld(), AVectorFieldVolume::StaticClass());
}

void AHolodeckBuoyantAgent::Tick(float DeltaSeconds) {
	Super::Tick(DeltaSeconds);
	if(octreeGlobal != nullptr) updateOctree(octreeLocal, octreeGlobal);
}

void AHolodeckBuoyantAgent::BeginDestroy() {
	Super::BeginDestroy();

	if(octreeLocal != nullptr) delete octreeLocal;
	if(octreeGlobal != nullptr) delete octreeGlobal;
}

void AHolodeckBuoyantAgent::ApplyBuoyantForce(){
    //Get all the values we need once
    FVector ActorLocation = GetActorLocation();
	FRotator ActorRotation = GetActorRotation();

	// Check to see how underwater we are
	FVector* points = SurfacePoints.GetData();
	int count = 0;
	for(int i=0;i<NumSurfacePoints;i++){
		FVector p_world = ActorLocation + ActorRotation.RotateVector(points[i]);
		if(p_world.Z < SurfaceLevel)
			count++;
	}
	float ratio = count*1.0 / NumSurfacePoints;

    // Get and apply Buoyant Force
	float BuoyantForce = Volume * Gravity * WaterDensity * ratio;
	FVector BuoyantVector = FVector(0, 0, BuoyantForce);
	BuoyantVector = ConvertLinearVector(BuoyantVector, ClientToUE);

    FVector COB_World = ActorLocation + ActorRotation.RotateVector(CenterBuoyancy + OffsetToOrigin);
	RootMesh->AddForceAtLocation(BuoyantVector, COB_World);

	FVector GravityVector = ConvertLinearVector(FVector(0, 0, -Gravity*MassInKG), ClientToUE);
	RootMesh->AddForceAtLocation(GravityVector, RootMesh->GetCenterOfMass());
}

// apply drag force to the AUV
void AHolodeckBuoyantAgent::ApplyDragForce() {
	FVector OceanCurrentsVel = FVector(0, 0, 0);
	if (VecFieldActorPtr != nullptr && GetActorLocation()[2] <= 0){
		AVectorFieldVolume* VecFieldVolume = Cast<AVectorFieldVolume>(VecFieldActorPtr);
		UVectorField* VecField = VecFieldVolume->GetVectorFieldComponent()->VectorField;

		FVector VecFieldLocation = VecFieldActorPtr->GetActorLocation();
		FVector VecFieldMin = VecField->Bounds.Min + VecFieldLocation;
		FVector VecFieldMax = VecField->Bounds.Max + VecFieldLocation;

		UVectorFieldStatic* VecFieldStatic = Cast<UVectorFieldStatic>(VecField);

		FVector VecFieldDimensions = FVector(VecFieldStatic->SizeX, VecFieldStatic->SizeY, VecFieldStatic->SizeZ);

		FVector GridPos = ((VecFieldDimensions - 1)/(VecFieldMax - VecFieldMin)) * (GetActorLocation() - VecFieldMin);

		OceanCurrentsVel = VecFieldStatic->FilteredSample(GridPos, FVector(1.0, 1.0, 1.0)) / 1000;

		//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,"Grid Pos: " + GridPos.ToString());
	}

	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,"Current Vel: " + OceanCurrentsVel.ToString());
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,"World Pos: " + GetActorLocation().ToString());
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, "-------------");


	FVector AUVVel = RootMesh->GetBodyInstance()->GetUnrealWorldVelocity() / 100.0; // in m/s
	FVector RelativeVel = AUVVel - OceanCurrentsVel;

	// density of water
	float rho = 1000.0;
	// drag force
	FVector DragForce = -0.5 * rho * RelativeVel.SizeSquared() * CoefficientOfDrag * AreaOfDrag * RelativeVel.GetSafeNormal();
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, OceanCurrentsVel.ToString());

	// You can use DrawDebug helpers and the log to help visualize and debug your trace queries.
	//DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + (OceanCurrentsVel * 1000), FColor::Red, false, 0.0f, 0, 5.0f);

	RootMesh->GetBodyInstance()->AddForce(DragForce, true, true);
}

/* // apply drag force to the AUV
void AHolodeckBuoyantAgent::ApplyDragOld() {
    if (OceanCurrentVelocityPtr != nullptr){
		FVector CurrentsVel = FVector(OceanCurrentVelocityPtr[0], OceanCurrentVelocityPtr[1], OceanCurrentVelocityPtr[2]);
		if (OceanCurrentVelocityPtr[3] > 0.0) {
			// FHitResult will hold all data returned by our line collision query
			FHitResult Hit;

			FVector TraceStart = GetActorLocation();
			FVector TraceEnd = TraceStart + FVector(0.0, 0.0, -100000.0);

			// Here we add ourselves to the ignored list so we won't block the trace
			FCollisionQueryParams QueryParams;
			QueryParams.AddIgnoredActor(this);

			// To run the query, you need a pointer to the current level, which you can get from an Actor with GetWorld()
			// UWorld()->LineTraceSingleByChannel runs a line trace and returns the first actor hit over the provided collision channel.
			GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, TraceChannelProperty, QueryParams);

			// If the trace hit something, bBlockingHit will be true,
			// and its fields will be filled with detailed info about what was hit
			if (Hit.bBlockingHit && IsValid(Hit.GetActor()))
			{
				// get our relative depth and scale the current velocity based on that
				float RelativeDepth = 1 - (TraceStart[2] / Hit.ImpactPoint[2]);
				CurrentsVel = CurrentsVel * RelativeDepth;
				//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::SanitizeFloat(RatioDepth));
			} else {
				CurrentsVel = FVector(0.0, 0.0, 0.0);
			}
		}

		FVector AUVVel = RootMesh->GetBodyInstance()->GetUnrealWorldVelocity() / 100.0; // in m/s
		FVector RelativeVel = AUVVel - CurrentsVel;

		// density of water
		float rho = 1000.0;
		// drag force
		FVector DragForce = -0.5 * rho * RelativeVel.SizeSquared() * CoefficientOfDrag * AreaOfDrag * RelativeVel.GetSafeNormal();
		//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, CurrentsVel.ToString());

		// You can use DrawDebug helpers and the log to help visualize and debug your trace queries.
		DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + (CurrentsVel * 1000), FColor::Red, false, 0.0f, 0, 5.0f);

		RootMesh->GetBodyInstance()->AddForce(DragForce, true, true);
    }
} */



void AHolodeckBuoyantAgent::ShowBoundingBox(float DeltaTime){
	FVector location = GetActorLocation() + GetActorRotation().RotateVector(OffsetToOrigin + CenterVehicle);
	DrawDebugBox(GetWorld(), location, BoundingBox.GetExtent(), GetActorQuat(), FColor::Red, false, DeltaTime, 0, 1);
}

void AHolodeckBuoyantAgent::ShowSurfacePoints(float DeltaTime){
	FVector ActorLocation = GetActorLocation();
	FRotator ActorRotation = GetActorRotation();
	FVector* points = SurfacePoints.GetData();

	for(int i=0;i<NumSurfacePoints;i++){
		FVector p_world = ActorLocation + ActorRotation.RotateVector(points[i]);
		DrawDebugPoint(GetWorld(), p_world, 5, FColor::Red, false, DeltaTime);
	}
}

Octree* AHolodeckBuoyantAgent::makeOctree(){
	if(octreeGlobal == nullptr){
		UE_LOG(LogHolodeck, Log, TEXT("HolodeckBuoyantAgent::Making Octree"));
		float OctreeMin = Octree::OctreeMin;
		float OctreeMax = Octree::OctreeMin;

		// Shrink to the smallest cube the actor fits in
		FVector center = BoundingBox.GetCenter() + GetActorLocation();
		float extent = BoundingBox.GetExtent().GetAbsMax()*2;
		while(OctreeMax < extent){
			OctreeMax *= 2;
		}

		// Otherwise, make the octrees
		octreeGlobal = Octree::makeOctree(center, OctreeMax, OctreeMin, GetName());
		if(octreeGlobal){
			octreeGlobal->isAgent = true;
			octreeGlobal->file = "AGENT";

			// Convert our global octree to a local one
			octreeLocal = cleanOctree(octreeGlobal);
		}
		else{
			UE_LOG(LogHolodeck, Warning, TEXT("HolodeckBuoyantAgent:: Failed to make Octree"));
		}

	}

	return octreeGlobal;
}

Octree* AHolodeckBuoyantAgent::cleanOctree(Octree* globalFrame){
	Octree* local = new Octree;
	local->loc = GetActorRotation().UnrotateVector(globalFrame->loc - GetActorLocation());
	local->normal = GetActorRotation().UnrotateVector(globalFrame->normal);

	for( Octree* tree : globalFrame->leaves){
		Octree* l = cleanOctree(tree);
		local->leaves.Add(l);
	}

	return local;
}

void AHolodeckBuoyantAgent::updateOctree(Octree* localFrame, Octree* globalFrame){
	globalFrame->loc = GetActorLocation() + GetActorRotation().RotateVector(localFrame->loc);
	globalFrame->normal = GetActorRotation().RotateVector(localFrame->normal);

	for(int i=0;i<globalFrame->leaves.Num();i++){
		updateOctree(localFrame->leaves[i], globalFrame->leaves[i]);
	}
}