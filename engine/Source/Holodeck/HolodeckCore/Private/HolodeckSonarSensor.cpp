// MIT License (c) 2021 BYU FRoStLab see LICENSE file

#include "Holodeck.h"
#include "Benchmarker.h"
#include "HolodeckBuoyantAgent.h"
#include "HolodeckSonarSensor.h"

float ATan2Approx(float y, float x){
    //http://pubs.opengroup.org/onlinepubs/009695399/functions/atan2.html
    //Volkan SALMA

    const float ONEQTR_PI = Pi / 4.0;
	const float THRQTR_PI = 3.0 * Pi / 4.0;
	float r, angle;
	float abs_y = fabs(y) + 1e-10f;      // kludge to prevent 0/0 condition
	if ( x < 0.0f )
	{
		r = (x + abs_y) / (abs_y - x);
		angle = THRQTR_PI;
	}
	else
	{
		r = (x - abs_y) / (x + abs_y);
		angle = ONEQTR_PI;
	}
	angle += (0.1963f * r * r - 0.9817f) * r;
	angle *= 180/Pi;
	if ( y < 0.0f )
		return( -angle );     // negate if in quad III or IV
	else
		return( angle );
}


// Allows sensor parameters to be set programmatically from client.
void UHolodeckSonarSensor::ParseSensorParms(FString ParmsJson) {
	Super::ParseSensorParms(ParmsJson);

	TSharedPtr<FJsonObject> JsonParsed;
	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(ParmsJson);
	if (FJsonSerializer::Deserialize(JsonReader, JsonParsed)) {
		if (JsonParsed->HasTypedField<EJson::Number>("MaxRange")) {
			MaxRange = JsonParsed->GetNumberField("MaxRange")*100;
		}

		if (JsonParsed->HasTypedField<EJson::Number>("InitOctreeRange")) {
			InitOctreeRange = JsonParsed->GetNumberField("InitOctreeRange")*100;
		}

		if (JsonParsed->HasTypedField<EJson::Number>("MinRange")) {
			MinRange = JsonParsed->GetNumberField("MinRange")*100;
		}

		if (JsonParsed->HasTypedField<EJson::Number>("Azimuth")) {
			Azimuth = JsonParsed->GetNumberField("Azimuth");
		}

		if (JsonParsed->HasTypedField<EJson::Number>("Elevation")) {
			Elevation = JsonParsed->GetNumberField("Elevation");
		}

		if (JsonParsed->HasTypedField<EJson::Number>("TicksPerCapture")) {
			TicksPerCapture = JsonParsed->GetIntegerField("TicksPerCapture");
		}
	}
	else {
		UE_LOG(LogHolodeck, Fatal, TEXT("UHolodeckSonarSensor::ParseSensorParms:: Unable to parse json."));
	}

	if(InitOctreeRange == 0){
		InitOctreeRange = MaxRange;
	}

	minAzimuth = -Azimuth/2;
	maxAzimuth = Azimuth/2;
	minElev = 90 - Elevation/2;
	maxElev = 90 + Elevation/2;

	sqrt3_2 = UKismetMathLibrary::Sqrt(3) / 2;
	sinOffset = UKismetMathLibrary::DegSin(FGenericPlatformMath::Min(Azimuth, Elevation)/2);
}

void UHolodeckSonarSensor::BeginDestroy() {
	Super::BeginDestroy();

	delete octree;
}

void UHolodeckSonarSensor::initOctree(){
	// We delay making trees till the message has been printed to the screen
	if(toMake.Num() != 0 && TickCounter > 2){
		UE_LOG(LogHolodeck, Log, TEXT("SonarSensor::Initial building num: %d"), toMake.Num());
		ParallelFor(toMake.Num(), [&](int32 i){
			toMake.GetData()[i]->load();
			toMake.GetData()[i]->unload();
		});
		toMake.Empty();
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Finished."));
	}

	// If we haven't made it yet
	if(octree == nullptr){
		// initialize small octree for each agent
		for(auto& agent : Controller->GetServer()->AgentMap){
			// skip ourselves
			if(agent.Value == this->GetAttachmentRootActor()) continue;
			AHolodeckBuoyantAgent* bouyantActor = static_cast<AHolodeckBuoyantAgent*>(agent.Value);
			Octree* l = bouyantActor->makeOctree();
			if(l) agents.Add(l);
		}
		
		// Ignore necessary agents to make world one
		for(auto& agent : Controller->GetServer()->AgentMap){
			AActor* actor = static_cast<AActor*>(agent.Value);
			Octree::ignoreActor(actor);
		}
		// make/load octree
		octree = Octree::makeEnvOctreeRoot();

		// Premake octrees within range
		FVector loc = this->GetComponentLocation();
		// Offset by size of OctreeMax radius to get everything in range
		float offset = InitOctreeRange + Octree::OctreeMax*FMath::Sqrt(3)/2;
		// recursively search for close leaves
		std::function<void(Octree*, TArray<Octree*>&)> findCloseLeaves;
		findCloseLeaves = [&offset, &loc, &findCloseLeaves](Octree* tree, TArray<Octree*>& list){
			if(tree->size == Octree::OctreeMax){
				if((loc - tree->loc).Size() < offset && !FPaths::FileExists(tree->file)){
					list.Add(tree);
				}
			}
			else{
				for(Octree* l : tree->leaves){
					findCloseLeaves(l, list);
				}
			}
		};
		findCloseLeaves(octree, toMake);
		if(toMake.Num() != 0)
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, FString::Printf(TEXT("Premaking %d Octrees, will take some time..."), toMake.Num()));

		// get all our Leaves ready
		bigLeaves.Reserve(1000);
		// We need enough foundLeaves to cover all possible threads
		// 1000 should be plenty
		foundLeaves.Reserve(1000);
		for(int i=0;i<1000;i++){
			foundLeaves.Add(TArray<Octree*>());
			foundLeaves[i].Reserve(10000);
		}
	}
}

void UHolodeckSonarSensor::viewLeaves(Octree* tree){
	if(tree->leaves.Num() == 0){
		DrawDebugPoint(GetWorld(), tree->loc, 5, FColor::Red, false, .03*TicksPerCapture);
	}
	else{
		for( Octree* l : tree->leaves){
			viewLeaves(l);
		}
	}
}

bool UHolodeckSonarSensor::inRange(Octree* tree){
	FTransform SensortoWorld = this->GetComponentTransform();
	// if it's not a leaf, we use a bigger search area
	float offset = 0;
	float radius = 0;
	if(tree->size != Octree::OctreeMin){
		radius = tree->size*sqrt3_2;
		offset = radius/sinOffset;
		SensortoWorld.AddToTranslation( -this->GetForwardVector()*offset );
	}

	// transform location to sensor frame
	// FVector locLocal = SensortoWorld.InverseTransformPositionNoScale(tree->loc);
	FVector locLocal = SensortoWorld.GetRotation().UnrotateVector(tree->loc-SensortoWorld.GetTranslation());

	// check if it's in range
	tree->locSpherical.X = locLocal.Size();
	if(MinRange+offset-radius >= tree->locSpherical.X || tree->locSpherical.X >= MaxRange+offset+radius) return false; 

	// check if azimuth is in
	tree->locSpherical.Y = ATan2Approx(-locLocal.Y, locLocal.X);
	if(minAzimuth >= tree->locSpherical.Y || tree->locSpherical.Y >= maxAzimuth) return false;

	// check if elevation is in
	tree->locSpherical.Z = ATan2Approx(locLocal.Size2D(), locLocal.Z);
	if(minElev >= tree->locSpherical.Z || tree->locSpherical.Z >= maxElev) return false;
	
	// otherwise it's in!
	return true;
}	

void UHolodeckSonarSensor::leavesInRange(Octree* tree, TArray<Octree*>& rLeaves, float stopAt){
	bool in = inRange(tree);
	if(in){
		if(tree->size == stopAt){
			if(stopAt == Octree::OctreeMin){
				// Compute contribution while we're parallelized
				// If no contribution, we don't have to add it in
				tree->normalImpact = GetComponentLocation() - tree->loc; 
				tree->normalImpact.Normalize();

				// compute contribution
				float val = FVector::DotProduct(tree->normal, tree->normalImpact);
				if(val > 0){
					tree->val = val;
					rLeaves.Add(tree);
				} 
			}
			else{
				rLeaves.Add(tree);
			}
			return;
		}

		for(Octree* l : tree->leaves){
			leavesInRange(l, rLeaves, stopAt);
		}
	}
	else if(tree->size >= Octree::OctreeMax){
		tree->unload();
	}
}

void UHolodeckSonarSensor::findLeaves(){
	// Empty everything out
	bigLeaves.Reset();
	for(auto& fl: foundLeaves){
		fl.Reset();
	}

	// FILTER TO GET THE bigLeaves WE WANT
	leavesInRange(octree, bigLeaves, Octree::OctreeMax);
	bigLeaves += agents;

	ParallelFor(bigLeaves.Num(), [&](int32 i){
		Octree* leaf = bigLeaves.GetData()[i];
		leaf->load();
		for(Octree* l : leaf->leaves)
			leavesInRange(l, foundLeaves.GetData()[i%1000], Octree::OctreeMin);
	});
}

void UHolodeckSonarSensor::TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	// We initialize this here to make sure all agents are loaded
	// This does nothing if it's already been loaded
	initOctree();

	TickCounter++;
	if(TickCounter == TicksPerCapture){
		TickCounter = 0;
	}
}