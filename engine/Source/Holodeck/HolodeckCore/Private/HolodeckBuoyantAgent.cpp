// MIT License (c) 2020 BYU PCCL see LICENSE file

#include "Holodeck.h"
#include "HolodeckBuoyantAgent.h"

void AHolodeckBuoyantAgent::InitializeAgent(){
	Super::InitializeAgent();

	// Set COM (have to do some calculation since it sets an offset)
	FVector COM_curr = GetActorRotation().UnrotateVector( RootMesh->GetCenterOfMass() - GetActorLocation() );
	RootMesh->SetCenterOfMass( CenterMass + OffsetToOrigin - COM_curr );
	// Set Mass
	RootMesh->SetMassOverrideInKg("", MassInKG);

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
}

void AHolodeckBuoyantAgent::Tick(float DeltaSeconds) {
	Super::Tick(DeltaSeconds);
	if(octreeGlobal.Num() == 0 && Server->octree.Num() != 0){
		UE_LOG(LogHolodeck, Warning, TEXT("Before %d"), Server->octree.Num());
		makeOctree();
		UE_LOG(LogHolodeck, Warning, TEXT("After %d"), Server->octree.Num());
	}
	updateOctree();
	// UE_LOG(LogHolodeck, Warning, TEXT("Local %s, \tGlobal %s"), *octreeLocal[0]->loc.ToString(), *octreeGlobal[0]->loc.ToString());
	// UE_LOG(LogHolodeck, Warning, TEXT("NumLocal %d, \tNumGlobal %d"), octreeLocal[0]->numLeafs(), octreeGlobal[0]->numLeafs());
}

void AHolodeckBuoyantAgent::BeginDestroy() {
	Super::BeginDestroy();

	if(octreeLocal.Num() != 0){
		for(Octree* t : octreeLocal){
			delete t;
		}
		octreeLocal.Empty();
	}
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
}

void AHolodeckBuoyantAgent::ShowBoundingBox(){
	FVector location = GetActorLocation() + GetActorRotation().RotateVector(OffsetToOrigin + CenterVehicle);
	DrawDebugBox(GetWorld(), location, BoundingBox.GetExtent(), GetActorQuat(), FColor::Red, false, 0.05, 0, 1);
}

void AHolodeckBuoyantAgent::ShowSurfacePoints(){
	FVector ActorLocation = GetActorLocation();
	FRotator ActorRotation = GetActorRotation();
	FVector* points = SurfacePoints.GetData();

	for(int i=0;i<NumSurfacePoints;i++){
		FVector p_world = ActorLocation + ActorRotation.RotateVector(points[i]);
		DrawDebugPoint(GetWorld(), p_world, 5, FColor::Red, false, 0.05);
	}
}

void AHolodeckBuoyantAgent::makeOctree(){
	if(octreeGlobal.Num() == 0){
		UE_LOG(LogHolodeck, Warning, TEXT("HolodeckAgent::Making Octree.."));
		int OctreeMin = Server->OctreeMin;
		int OctreeMax = Server->OctreeMax;

		// Otherwise, make the octrees
		FVector nCells = (BoundingBox.Max - BoundingBox.Min) / OctreeMax;
		// UE_LOG(LogHolodeck, Warning, TEXT("nCells: %s"), *nCells.ToString());
		for(int i = 0; i < nCells.X; i++) {
			for(int j = 0; j < nCells.Y; j++) {
				for(int k = 0; k < nCells.Z; k++) {
					FVector center = FVector(i*OctreeMax, j*OctreeMax, k*OctreeMax) + BoundingBox.Min + GetActorLocation();
					// UE_LOG(LogHolodeck, Warning, TEXT("center: %s"), *center.ToString());
					// UE_LOG(LogHolodeck, Warning, TEXT("bb: %s"), *BoundingBox.Min.ToString());
					// UE_LOG(LogHolodeck, Warning, TEXT("loc: %s"), *GetActorLocation().ToString());
					Octree::makeOctree(center, OctreeMax, GetWorld(), octreeGlobal, OctreeMin);
				}
			}
		}
		Server->octree += octreeGlobal;

		// Clean out the octree
		for( Octree* tree : octreeGlobal){
			cleanOctree(tree, octreeLocal);
		}
		// UE_LOG(LogHolodeck, Warning, TEXT("Global size: %d, Local Size %d"), octreeGlobal.Num(), octreeLocal.Num());
	}
}

void AHolodeckBuoyantAgent::cleanOctree(Octree* globalFrame, TArray<Octree*>& results){
	Octree* octree = new Octree;
	octree->loc = GetActorRotation().UnrotateVector(globalFrame->loc - GetActorLocation());
	octree->normal = GetActorRotation().UnrotateVector(globalFrame->normal);

	for( Octree* tree : globalFrame->leafs){
		cleanOctree(tree, octree->leafs);
	}

	results.Add(octree);
}

void AHolodeckBuoyantAgent::updateOctree(){
	for(int i=0;i<octreeGlobal.Num();i++){
		updateOctree(octreeLocal[i], octreeGlobal[i]);
	}
}

void AHolodeckBuoyantAgent::updateOctree(Octree* localFrame, Octree* globalFrame){
	globalFrame->loc = GetActorLocation() + GetActorRotation().RotateVector(localFrame->loc);
	globalFrame->normal = GetActorRotation().RotateVector(localFrame->normal);

	for(int i=0;i<globalFrame->leafs.Num();i++){
		updateOctree(localFrame->leafs[i], globalFrame->leafs[i]);
	}
}