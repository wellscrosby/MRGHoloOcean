// MIT License (c) 2019 BYU PCCL see LICENSE file

#include "Holodeck.h"
#include "Benchmarker.h"
#include "HolodeckBuoyantAgent.h"
#include "SonarSensor.h"
#pragma warning (disable : 4101)

float ATan2Approx(float y, float x)
{
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

USonarSensor::USonarSensor() {
	SensorName = "SonarSensor";
}

void USonarSensor::BeginDestroy() {
	Super::BeginDestroy();

	delete octree;

	delete[] count;
}

// Allows sensor parameters to be set programmatically from client.
void USonarSensor::ParseSensorParms(FString ParmsJson) {
	Super::ParseSensorParms(ParmsJson);

	TSharedPtr<FJsonObject> JsonParsed;
	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(ParmsJson);
	if (FJsonSerializer::Deserialize(JsonReader, JsonParsed)) {

		// For handling noise
		if (JsonParsed->HasTypedField<EJson::Number>("AddSigma")) {
			addNoise.initSigma(JsonParsed->GetNumberField("AddSigma"));
		}
		if (JsonParsed->HasTypedField<EJson::Number>("AddCov")) {
			addNoise.initCov(JsonParsed->GetNumberField("AddCov"));
		}
		if (JsonParsed->HasTypedField<EJson::Number>("MultSigma")) {
			multNoise.initSigma(JsonParsed->GetNumberField("MultSigma"));
		}
		if (JsonParsed->HasTypedField<EJson::Number>("MultCov")) {
			multNoise.initCov(JsonParsed->GetNumberField("MultCov"));
		}
		if (JsonParsed->HasTypedField<EJson::Number>("AzimuthSigma")) {
			aziNoise.initSigma(JsonParsed->GetNumberField("AzimuthSigma"));
		}
		if (JsonParsed->HasTypedField<EJson::Number>("AzimuthCov")) {
			aziNoise.initCov(JsonParsed->GetNumberField("AzimuthCov"));
		}
		if (JsonParsed->HasTypedField<EJson::Number>("RangeSigma")) {
			rNoise.initSigma(JsonParsed->GetNumberField("RangeSigma")*100);
		}
		if (JsonParsed->HasTypedField<EJson::Number>("RangeCov")) {
			rNoise.initCov(JsonParsed->GetNumberField("RangeCov")*100*100);
		}

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

		if (JsonParsed->HasTypedField<EJson::Number>("BinsRange")) {
			BinsRange = JsonParsed->GetIntegerField("BinsRange");
		}

		if (JsonParsed->HasTypedField<EJson::Number>("BinsAzimuth")) {
			BinsAzimuth = JsonParsed->GetIntegerField("BinsAzimuth");
		}

		if (JsonParsed->HasTypedField<EJson::Number>("BinsElevation")) {
			BinsElevation = JsonParsed->GetIntegerField("BinsElevation");
		}

		if (JsonParsed->HasTypedField<EJson::Boolean>("ViewRegion")) {
			ViewRegion = JsonParsed->GetBoolField("ViewRegion");
		}

		if (JsonParsed->HasTypedField<EJson::Number>("ViewOctree")) {
			ViewOctree = JsonParsed->GetIntegerField("ViewOctree");
		}

		if (JsonParsed->HasTypedField<EJson::Number>("TicksPerCapture")) {
			TicksPerCapture = JsonParsed->GetIntegerField("TicksPerCapture");
		}

	}
	else {
		UE_LOG(LogHolodeck, Fatal, TEXT("USonarSensor::ParseSensorParms:: Unable to parse json."));
	}

	if(InitOctreeRange == 0){
		InitOctreeRange = MaxRange;
	}
	if(BinsElevation == 0){
		BinsElevation = Elevation*10;
	}
}

void USonarSensor::initOctree(){
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
		std::function<void(Octree*, TArray<Octree*>&)> findCloseLeafs;
		findCloseLeafs = [&offset, &loc, &findCloseLeafs](Octree* tree, TArray<Octree*>& list){
			if(tree->size == Octree::OctreeMax){
				if((loc - tree->loc).Size() < offset && !FPaths::FileExists(tree->file)){
					list.Add(tree);
				}
			}
			else{
				for(Octree* l : tree->leafs){
					findCloseLeafs(l, list);
				}
			}
		};
		findCloseLeafs(octree, toMake);
		if(toMake.Num() != 0)
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, FString::Printf(TEXT("Premaking %d Octrees, will take some time..."), toMake.Num()));

		// get all our leafs ready
		leafs.Reserve(1000);
		// We need enough templeafs to cover all possible threads
		// 1000 should be plenty
		tempLeafs.Reserve(1000);
		for(int i=0;i<1000;i++){
			tempLeafs.Add(TArray<Octree*>());
			tempLeafs[i].Reserve(10000);
		}
		for(int i=0;i<BinsAzimuth*BinsElevation;i++){
			sortedLeafs.Add(TArray<Octree*>());
			sortedLeafs[i].Reserve(10000);
		}
	}
}

void USonarSensor::InitializeSensor() {
	Super::InitializeSensor();
	
	// Get size of each bin
	RangeRes = (MaxRange - MinRange) / BinsRange;
	AzimuthRes = Azimuth / BinsAzimuth;
	ElevRes = Elevation / BinsElevation;

	minAzimuth = -Azimuth/2;
	maxAzimuth = Azimuth/2;
	minElev = 90 - Elevation/2;
	maxElev = 90 + Elevation/2;

	sinOffset = UKismetMathLibrary::DegSin(FGenericPlatformMath::Min(Azimuth, Elevation)/2);
	sqrt2 = UKismetMathLibrary::Sqrt(2);
	
	// setup count of each bin
	count = new int32[BinsRange*BinsAzimuth]();
}

bool USonarSensor::inRange(Octree* tree){
	FTransform SensortoWorld = this->GetComponentTransform();
	// if it's not a leaf, we use a bigger search area
	float offset = 0;
	float radius = 0;
	if(tree->size != Octree::OctreeMin){
		radius = tree->size/sqrt2;
		offset = radius/sinOffset;
		SensortoWorld.AddToTranslation( -this->GetForwardVector()*offset );
	}

	// transform location to sensor frame
	// FVector locLocal = SensortoWorld.InverseTransformPositionNoScale(tree->loc);
	FVector locLocal = SensortoWorld.GetRotation().UnrotateVector(tree->loc-SensortoWorld.GetTranslation());

	// check if it's in range
	tree->locSpherical.X = locLocal.Size() + rNoise.sampleFloat();
	if(MinRange+offset-radius >= tree->locSpherical.X || tree->locSpherical.X >= MaxRange+offset+radius) return false; 

	// check if azimuth is in
	tree->locSpherical.Y = ATan2Approx(-locLocal.Y, locLocal.X) + aziNoise.sampleFloat();
	if(minAzimuth >= tree->locSpherical.Y || tree->locSpherical.Y >= maxAzimuth) return false;

	// check if elevation is in
	tree->locSpherical.Z = ATan2Approx(locLocal.Size2D(), locLocal.Z);
	if(minElev >= tree->locSpherical.Z || tree->locSpherical.Z >= maxElev) return false;
	
	// otherwise it's in!
	return true;
}	

void USonarSensor::leafsInRange(Octree* tree, TArray<Octree*>& rLeafs, float stopAt){
	bool in = inRange(tree);
	if(in){
		if(tree->size == stopAt){
			if(stopAt == Octree::OctreeMin){
				// Compute contribution while we're parallelized
				// If no contribution, we don't have to add it in
				FVector normalImpact = GetComponentLocation() - tree->loc; 
				normalImpact.Normalize();

				// compute contribution
				float val = FVector::DotProduct(tree->normal, normalImpact);
				if(val > 0){
					tree->val = val;

					// Compute bins while we're parallelized
					tree->idx.Y = (int32)((tree->locSpherical.Y - minAzimuth)/ AzimuthRes);
					tree->idx.Z = (int32)((tree->locSpherical.Z - minElev)/ ElevRes);
					// Sometimes we get float->int rounding errors
					if(tree->idx.Y == BinsAzimuth) --tree->idx.Y;

					rLeafs.Add(tree);
				} 
			}
			else{
				rLeafs.Add(tree);
			}
			return;
		}

		for(Octree* l : tree->leafs){
			leafsInRange(l, rLeafs, stopAt);
		}
	}
	else if(tree->size >= Octree::OctreeMax){
		tree->unload();
	}
}

FVector spherToEuc(float r, float theta, float phi, FTransform SensortoWorld){
	float x = r*UKismetMathLibrary::DegSin(phi)*UKismetMathLibrary::DegCos(theta);
	float y = r*UKismetMathLibrary::DegSin(phi)*UKismetMathLibrary::DegSin(theta);
	float z = r*UKismetMathLibrary::DegCos(phi);
	return UKismetMathLibrary::TransformLocation(SensortoWorld, FVector(x, y, z));
}

void USonarSensor::viewLeafs(Octree* tree){
	if(tree->leafs.Num() == 0){
		DrawDebugPoint(GetWorld(), tree->loc, 5, FColor::Red, false, .03*TicksPerCapture);
	}
	else{
		for( Octree* l : tree->leafs){
			viewLeafs(l);
		}
	}
}
void USonarSensor::TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	// We initialize this here to make sure all agents are loaded
	// This does nothing if it's already been loaded
	initOctree();

	TickCounter++;
	if(TickCounter == TicksPerCapture){
		// reset things and get ready
		TickCounter = 0;
		float* result = static_cast<float*>(Buffer);
		std::fill(result, result+BinsRange*BinsAzimuth, 0);
		std::fill(count, count+BinsRange*BinsAzimuth, 0);
		leafs.Reset();
		for(auto& tl: tempLeafs){
			tl.Reset();
		}
		for(auto& sl: sortedLeafs){
			sl.Reset();
		}


		// FILTER TO GET THE LEAFS WE WANT
		leafsInRange(octree, leafs, Octree::OctreeMax);
		leafs += agents;

		ParallelFor(leafs.Num(), [&](int32 i){
			Octree* leaf = leafs.GetData()[i];
			leaf->load();
			for(Octree* l : leaf->leafs)
				leafsInRange(l, tempLeafs.GetData()[i%1000], Octree::OctreeMin);
		});
		

		// SORT THEM INTO AZIMUTH/ELEVATION BINS
		int32 idx;
		for(TArray<Octree*>& bin : tempLeafs){
			for(Octree* l : bin){
				idx = l->idx.Z*BinsAzimuth + l->idx.Y;
				sortedLeafs[idx].Emplace(l);
			}
		}


		// HANDLE SHADOWING
		float eps = 16;
		ParallelFor(BinsAzimuth*BinsElevation, [&](int32 i){
			TArray<Octree*>& binLeafs = sortedLeafs.GetData()[i]; 

			// sort from closest to farthest
			binLeafs.Sort([](const Octree& a, const Octree& b){
				return a.locSpherical.X < b.locSpherical.X;
			});

			// Get the closest cluster in the bin
			int32 idx;
			float diff, cos_i, sin_i, temp, cos_t, z_t, R;
			float z_i = sos_water * density_water;
			Octree* jth;
			for(int j=0;j<binLeafs.Num()-1;j++){
				jth = binLeafs.GetData()[j];
				
				jth->idx.X = (int32)((jth->locSpherical.X - MinRange) / RangeRes);
				idx = jth->idx.X*BinsAzimuth + jth->idx.Y;
				// cos_i = jth->val;
				// sin_i = FMath::Sqrt(1 - cos_i*cos_i);
				// temp = jth->sos*sin_i / sos_water;
				// cos_t = FMath::Sqrt(1 - temp*temp); // Getting nan's b/c past critical angle...
				// temp = (z_t*cos_i - z_i*cos_t) / (z_t*cos_i + z_i*cos_t);
				// UE_LOG(LogHolodeck, Warning, TEXT("sin_i: %f, cos_t: %f, R: %f"), sin_i, cos_t, temp);
				z_t = jth->sos * jth->density;
				R = (z_t - z_i) / (z_t + z_i);
				result[idx] += jth->val * R; // * temp*temp;
				// result[idx] += jth->val;
				++count[idx];
				
				// diff = FVector::Dist(jth->loc, binLeafs.GetData()[j+1]->loc);
				diff = FMath::Abs(jth->locSpherical.X - binLeafs.GetData()[j+1]->locSpherical.X);
				if(diff > eps) break;
			}

		});

		
		// MOVE THEM INTO BUFFER
		for (int i = 0; i < BinsRange*BinsAzimuth; i++) {
			if(count[i] != 0){
				result[i] *= (1 + multNoise.sampleFloat())/count[i];
				result[i] += addNoise.sampleRayleigh();
			}
			else{
				result[i] = addNoise.sampleRayleigh();
			}
		}


		// draw points inside our region
		if(ViewOctree >= -1){
			for( TArray<Octree*> bins : sortedLeafs){
				for( Octree* l : bins){
					if(ViewOctree == -1 || ViewOctree == l->idx.Y)
						DrawDebugPoint(GetWorld(), l->loc, 5, FColor::Red, false, DeltaTime*TicksPerCapture);
				}
			}
		}

		// draw outlines of our region
		if(ViewRegion){
			FTransform tran = this->GetComponentTransform();
			float debugThickness = 3.0f;
			
			// range lines
			DrawDebugLine(GetWorld(), spherToEuc(MinRange, minAzimuth, minElev, tran), spherToEuc(MaxRange, minAzimuth, minElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, debugThickness);
			DrawDebugLine(GetWorld(), spherToEuc(MinRange, minAzimuth, maxElev, tran), spherToEuc(MaxRange, minAzimuth, maxElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, debugThickness);
			DrawDebugLine(GetWorld(), spherToEuc(MinRange, maxAzimuth, minElev, tran), spherToEuc(MaxRange, maxAzimuth, minElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, debugThickness);
			DrawDebugLine(GetWorld(), spherToEuc(MinRange, maxAzimuth, maxElev, tran), spherToEuc(MaxRange, maxAzimuth, maxElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, debugThickness);

			// azimuth lines (should be arcs, we're being lazy)
			DrawDebugLine(GetWorld(), spherToEuc(MinRange, minAzimuth, minElev, tran), spherToEuc(MinRange, maxAzimuth, minElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, debugThickness);
			DrawDebugLine(GetWorld(), spherToEuc(MinRange, minAzimuth, maxElev, tran), spherToEuc(MinRange, maxAzimuth, maxElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, debugThickness);
			DrawDebugLine(GetWorld(), spherToEuc(MaxRange, minAzimuth, minElev, tran), spherToEuc(MaxRange, maxAzimuth, minElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, debugThickness);
			DrawDebugLine(GetWorld(), spherToEuc(MaxRange, minAzimuth, maxElev, tran), spherToEuc(MaxRange, maxAzimuth, maxElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, debugThickness);

			// elevation lines (should be arcs, we're being lazy)
			DrawDebugLine(GetWorld(), spherToEuc(MinRange, minAzimuth, minElev, tran), spherToEuc(MinRange, minAzimuth, maxElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, debugThickness);
			DrawDebugLine(GetWorld(), spherToEuc(MinRange, maxAzimuth, minElev, tran), spherToEuc(MinRange, maxAzimuth, maxElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, debugThickness);
			DrawDebugLine(GetWorld(), spherToEuc(MaxRange, minAzimuth, minElev, tran), spherToEuc(MaxRange, minAzimuth, maxElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, debugThickness);
			DrawDebugLine(GetWorld(), spherToEuc(MaxRange, maxAzimuth, minElev, tran), spherToEuc(MaxRange, maxAzimuth, maxElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, debugThickness);
		}		
	}
}
