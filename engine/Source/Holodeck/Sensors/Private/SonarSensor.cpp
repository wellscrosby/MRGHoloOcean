// MIT License (c) 2019 BYU PCCL see LICENSE file

#include "Holodeck.h"
#include "Benchmarker.h"
#include "HolodeckBuoyantAgent.h"
#include "SonarSensor.h"
// #pragma warning (disable : 4101)

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

	for(Octree* t : octree){
		delete t;
	}
	octree.Empty();

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

		if (JsonParsed->HasTypedField<EJson::Number>("MaxRange")) {
			MaxRange = JsonParsed->GetNumberField("MaxRange")*100;
		}

		if (JsonParsed->HasTypedField<EJson::Number>("InitOctreeRange")) {
			MaxRange = JsonParsed->GetNumberField("InitOctreeRange")*100;
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

		if (JsonParsed->HasTypedField<EJson::Boolean>("ViewRegion")) {
			ViewRegion = JsonParsed->GetBoolField("ViewRegion");
		}

		if (JsonParsed->HasTypedField<EJson::Boolean>("ViewOctree")) {
			ViewOctree = JsonParsed->GetBoolField("ViewOctree");
		}

		if (JsonParsed->HasTypedField<EJson::Number>("TicksPerCapture")) {
			TicksPerCapture = JsonParsed->GetIntegerField("TicksPerCapture");
		}

	}
	else {
		UE_LOG(LogHolodeck, Fatal, TEXT("USonarSensor::ParseSensorParms:: Unable to parse json."));
	}
}

void USonarSensor::initOctree(){
	if(octree.Num() == 0){
		// This is done here b/c Server doesn't have inheritance info of AHolodeckAgent
		for(auto& agent : Controller->GetServer()->AgentMap){
			AActor* actor = static_cast<AActor*>(agent.Value);
			Octree::ignoreActor(actor);
		}
		// make/load octree
		octree = Octree::getOctreeRoots(GetWorld());
		// Octree::resetParams();

		// initialize small octree for each agent
		// for(auto& agent : Controller->GetServer()->AgentMap){
		// 	AHolodeckBuoyantAgent* bouyantActor = static_cast<AHolodeckBuoyantAgent*>(agent.Value);
		// 	bouyantActor->makeOctree();
		// }

		// get all our leafs ready
		// TODO: calculate what these values should be
		// TODO: This needs to be moved somewhere to make sure it happens to every sonar
		FVector loc = this->GetComponentLocation();
		TArray<Octree*> toMake;
		for(Octree* tree : octree){
			if((loc - tree->loc).Size() < InitOctreeRange){
				toMake.Add(tree);
			}
		}
		ParallelFor(toMake.Num(), [&](int32 i){
			toMake.GetData()[i]->load();
			toMake.GetData()[i]->unload();
		});


		leafs.Reserve(10000);
		tempLeafs.Reserve(octree.Num());
		for(int i=0;i<octree.Num();i++){
			tempLeafs.Add(TArray<Octree*>());
			tempLeafs[i].Reserve(1000);
		}
		for(int i=0;i<BinsAzimuth*BinsElev;i++){
			sortedLeafs.Add(TArray<Octree*>());
			sortedLeafs[i].Reserve(200);
		}
	}
}
void USonarSensor::InitializeSensor() {
	Super::InitializeSensor();

	BinsElev = (int) Elevation;
	
	// Get size of each bin
	RangeRes = (MaxRange - MinRange) / BinsRange;
	AzimuthRes = Azimuth / BinsAzimuth;
	ElevRes = Elevation / BinsElev;

	minAzimuth = -Azimuth/2;
	maxAzimuth = Azimuth/2;
	minElev = 90 - Elevation/2;
	maxElev = 90 + Elevation/2;

	sinOffset = UKismetMathLibrary::DegSin(FGenericPlatformMath::Min(Azimuth, Elevation)/2);
	sqrt2 = UKismetMathLibrary::Sqrt(2);
	

	// setup leaves for later
	count = new int32[BinsRange*BinsAzimuth]();
}

bool USonarSensor::inRange(Octree* tree){
	FTransform SensortoWorld = this->GetComponentTransform();
	// if it's not a leaf, we use a bigger search area
	float offset = 0;
	if(tree->size != Octree::OctreeMin){
		float radius = tree->size/sqrt2;
		offset = radius/sinOffset;
		SensortoWorld.AddToTranslation( -this->GetForwardVector()*offset );
		offset += radius;
	}

	// transform location to sensor frame
	// FVector locLocal = SensortoWorld.InverseTransformPositionNoScale(tree->loc);
	FVector locLocal = SensortoWorld.GetRotation().UnrotateVector(tree->loc-SensortoWorld.GetTranslation());

	// check if it's in range
	tree->locSpherical.X = locLocal.Size();
	if(MinRange >= tree->locSpherical.X || tree->locSpherical.X >= MaxRange+offset) return false; 

	// check if azimuth is in
	tree->locSpherical.Y = ATan2Approx(-locLocal.Y, locLocal.X);
	if(minAzimuth >= tree->locSpherical.Y || tree->locSpherical.Y >= maxAzimuth) return false;

	// check if elevation is in
	tree->locSpherical.Z = ATan2Approx(FVector2D(locLocal.X, locLocal.Y).Size(), locLocal.Z);
	if(minElev >= tree->locSpherical.Z || tree->locSpherical.Z >= maxElev) return false;
	
	// otherwise it's in!
	return true;
}	

void USonarSensor::leafsInRange(Octree* tree, TArray<Octree*>& rLeafs){
	bool in = inRange(tree);
	if(in){
		if(tree->size == Octree::OctreeMin){
			rLeafs.Add(tree);
			return;
		}

		if(tree->size == Octree::OctreeMax){
			tree->load();
		}

		for(Octree* l : tree->leafs){
			leafsInRange(l, rLeafs);
		}
	}
	else if(tree->size == Octree::OctreeMax){
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
		// for(Octree* t: octree){
		// 	UE_LOG(LogHolodeck, Warning, TEXT("Octree p: %s"), *t->loc.ToString());
		// 	viewLeafs(t);
		// }
		ParallelFor(octree.Num(), [&](int32 i){
			leafsInRange(octree.GetData()[i], tempLeafs.GetData()[i]);
		});
		for(auto& tl : tempLeafs){
			leafs += tl;
		}

		// GET THE DOT PRODUCT, REMOVE BACKSIDE OF ANYTHING
		FVector compLoc = this->GetComponentLocation();
		ParallelFor(leafs.Num(), [&](int32 i){
			Octree* l = leafs.GetData()[i]; 

			// Compute impact normal
			FVector normalImpact = compLoc - l->loc; 
			normalImpact /= normalImpact.Size();

			// compute contribution
			float val = FVector::DotProduct(l->normal, normalImpact);
			if(val > 0) l->val = val;
			else leafs.GetData()[i] = nullptr;
		});


		// SORT THEM INTO AZIMUTH/ELEVATION BINS
		for(Octree* l : leafs){
			if(l == nullptr) continue;

			int32 aBin = (int)((l->locSpherical.Y - minAzimuth)/ AzimuthRes);
			int32 eBin = (int)((l->locSpherical.Z - minElev)/ ElevRes);
			// Sometimes we get float->int rounding errors
			if(aBin == BinsAzimuth) --aBin;

			int32 idx = eBin*BinsAzimuth + aBin;
			sortedLeafs[idx].Add(l);
		}


		// HANDLE SHADOWING
		// TODO: Do these degree params need to be check for larger octree sizes?
		float shadowAngle = 1;
		float shadowCos = -FMath::Cos(shadowAngle*Pi/180);
		ParallelFor(BinsAzimuth*BinsElev, [&](int32 i){
			TArray<Octree*>& binLeafs = sortedLeafs.GetData()[i]; 

			// sort from closest to farthest
			binLeafs.Sort([](const Octree& a, const Octree& b){
				return a.locSpherical.X < b.locSpherical.X;
			});

			int32 j=0,k,rBin,aBin,idx;
			float a,aa,b,bb,c,cos;
			Octree *close, *other;
			// Iterate through, adding in contributions
			while(j < binLeafs.Num()){
				k = binLeafs.Num()-1;
				close = binLeafs.GetData()[j];

				// Compute what bin it goes in
				aBin = (int)((close->locSpherical.Y - minAzimuth)/ AzimuthRes);
				rBin = (int)((close->locSpherical.X - MinRange) / RangeRes);
				idx = rBin*BinsAzimuth + aBin;

				// TODO: use sigmoid here?
				result[idx] += close->val;
				++count[idx];

				// remove ones that are in the shadow of bin j
				a = close->locSpherical.X;
				aa = a*a;
				while(k > j){
					other = binLeafs.GetData()[k];
					bb = FVector::DistSquared(close->loc, other->loc);
					b = FMath::Sqrt(bb);
					c = other->locSpherical.X;
					cos = (aa + bb - c*c) / (2*a*b);

					if(cos < shadowCos) binLeafs.RemoveAt(k);

					--k;
				}
				++j;
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
		if(ViewOctree){
			for( Octree* l : leafs){
				if(l != nullptr) DrawDebugPoint(GetWorld(), l->loc, 5, FColor::Red, false, DeltaTime*TicksPerCapture);
			}
		}
		// draw outlines of our region
		if(ViewRegion){
			FTransform tran = this->GetComponentTransform();
			
			// range lines
			DrawDebugLine(GetWorld(), spherToEuc(MinRange, minAzimuth, minElev, tran), spherToEuc(MaxRange, minAzimuth, minElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, 1.f);
			DrawDebugLine(GetWorld(), spherToEuc(MinRange, minAzimuth, maxElev, tran), spherToEuc(MaxRange, minAzimuth, maxElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, 1.f);
			DrawDebugLine(GetWorld(), spherToEuc(MinRange, maxAzimuth, minElev, tran), spherToEuc(MaxRange, maxAzimuth, minElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, 1.f);
			DrawDebugLine(GetWorld(), spherToEuc(MinRange, maxAzimuth, maxElev, tran), spherToEuc(MaxRange, maxAzimuth, maxElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, 1.f);

			// azimuth lines (should be arcs, we're being lazy)
			DrawDebugLine(GetWorld(), spherToEuc(MinRange, minAzimuth, minElev, tran), spherToEuc(MinRange, maxAzimuth, minElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, 1.f);
			DrawDebugLine(GetWorld(), spherToEuc(MinRange, minAzimuth, maxElev, tran), spherToEuc(MinRange, maxAzimuth, maxElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, 1.f);
			DrawDebugLine(GetWorld(), spherToEuc(MaxRange, minAzimuth, minElev, tran), spherToEuc(MaxRange, maxAzimuth, minElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, 1.f);
			DrawDebugLine(GetWorld(), spherToEuc(MaxRange, minAzimuth, maxElev, tran), spherToEuc(MaxRange, maxAzimuth, maxElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, 1.f);

			// elevation lines (should be arcs, we're being lazy)
			DrawDebugLine(GetWorld(), spherToEuc(MinRange, minAzimuth, minElev, tran), spherToEuc(MinRange, minAzimuth, maxElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, 1.f);
			DrawDebugLine(GetWorld(), spherToEuc(MinRange, maxAzimuth, minElev, tran), spherToEuc(MinRange, maxAzimuth, maxElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, 1.f);
			DrawDebugLine(GetWorld(), spherToEuc(MaxRange, minAzimuth, minElev, tran), spherToEuc(MaxRange, minAzimuth, maxElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, 1.f);
			DrawDebugLine(GetWorld(), spherToEuc(MaxRange, maxAzimuth, minElev, tran), spherToEuc(MaxRange, maxAzimuth, maxElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, 1.f);
		}		
	}
}
