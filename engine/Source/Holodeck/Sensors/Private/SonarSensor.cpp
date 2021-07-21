// MIT License (c) 2019 BYU PCCL see LICENSE file

#include "Holodeck.h"
#include "Benchmarker.h"
#include "HolodeckBuoyantAgent.h"
#include "SonarSensor.h"

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

		if (JsonParsed->HasTypedField<EJson::Boolean>("ViewDebug")) {
			ViewDebug = JsonParsed->GetBoolField("ViewDebug");
		}

		if (JsonParsed->HasTypedField<EJson::Number>("TicksPerCapture")) {
			TicksPerCapture = JsonParsed->GetIntegerField("TicksPerCapture");
		}

	}
	else {
		UE_LOG(LogHolodeck, Fatal, TEXT("USonarSensor::ParseSensorParms:: Unable to parse json."));
	}
}

void USonarSensor::InitializeSensor() {
	Super::InitializeSensor();

	// Ignore all agents thus far
	// This is done here b/c Server doesn't have inheritance info of AHolodeckAgent
	for(auto& agent : Controller->GetServer()->AgentMap){
		AActor* actor = static_cast<AActor*>(agent.Value);
		Octree::ignoreActor(actor);
	}
	// make/load octree
	Controller->GetServer()->makeOctree(GetWorld());
	// remove all ignored agents (agents will make their own octree soon)
	Octree::resetParams();
	OctreeMax = Controller->GetServer()->OctreeMax;
	
	// Get size of each bin
	RangeRes = (MaxRange - MinRange) / BinsRange;
	AzimuthRes = Azimuth / BinsAzimuth;

	minAzimuth = -Azimuth/2;
	maxAzimuth = Azimuth/2;
	minElev = 90 - Elevation/2;
	maxElev = 90 + Elevation/2;

	sinOffset = UKismetMathLibrary::DegSin(FGenericPlatformMath::Min(Azimuth, Elevation)/2);
	sqrt2 = UKismetMathLibrary::Sqrt(2);

	// setup leaves for later
	// TODO: calculate what these values should be
	TArray<Octree*>& octree = getOctree();
	leafs.Reserve(10000);
	tempLeafs.Reserve(octree.Num()+30);
	for(int i=0;i<octree.Num()+30;i++){
		tempLeafs.Add(TArray<Octree*>());
		tempLeafs[i].Reserve(1000);
	}
	count = new int32[BinsRange*BinsAzimuth]();
}

bool USonarSensor::inRange(Octree* tree, float size){
	FTransform SensortoWorld = this->GetComponentTransform();
	// if it's not a leaf, we use a bigger search area
	float offset = 0;
	if(tree->leafs.Num() != 0){
		float radius = size/sqrt2;
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

void USonarSensor::leafsInRange(Octree* tree, TArray<Octree*>& rLeafs, float size){
	bool in = inRange(tree, size);
	if(in){
		if(tree->leafs.Num() == 0){
			rLeafs.Add(tree);
		}
		else{
			for(Octree* l : tree->leafs){
				leafsInRange(l, rLeafs, size/2);
			}
		}
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
	TickCounter++;
	if(TickCounter == TicksPerCapture){
		// reset things and get ready
		float* result = static_cast<float*>(Buffer);
		std::fill(result, result+BinsRange*BinsAzimuth, 0);
		TArray<Octree*>& octree = getOctree();

		// FILTER TO GET THE LEAFS WE WANT
		ParallelFor(octree.Num(), [&](int32 i){
			leafsInRange(octree.GetData()[i], tempLeafs.GetData()[i], OctreeMax);
		});
		for(auto& tl : tempLeafs){
			leafs += tl;
		}

		// SORT THEM & RUN CALCULATIONS
		FVector compLoc = this->GetComponentLocation();
		ParallelFor(leafs.Num(), [&](int32 i){
			Octree* l = leafs.GetData()[i];
			// find what bin they must go in
			int32 rBin = (int)((l->locSpherical[0] - MinRange) / RangeRes);
			int32 aBin = (int)((l->locSpherical[1] - minAzimuth)/ AzimuthRes);
			int32 idx = rBin*BinsAzimuth + aBin;

			// Compute impact normal
			FVector normalImpact = compLoc - l->loc; 
			normalImpact /= normalImpact.Size();
			l->locSpherical = normalImpact;

			// compute contribution
			float val = FVector::DotProduct(l->normal, normalImpact);
			if(val > 0){
				// TODO: use sigmoid here?
				result[idx] += val;
				++count[idx];
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

		// draw outlines of our region
		if(ViewDebug){
			for( Octree* l : leafs){
				DrawDebugPoint(GetWorld(), l->loc, 5, FColor::Red, false, .03*TicksPerCapture);
			}
			FTransform tran = this->GetComponentTransform();
			
			// range lines
			DrawDebugLine(GetWorld(), spherToEuc(MinRange, minAzimuth, minElev, tran), spherToEuc(MaxRange, minAzimuth, minElev, tran), FColor::Green, false, .03*TicksPerCapture, ECC_WorldStatic, 1.f);
			DrawDebugLine(GetWorld(), spherToEuc(MinRange, minAzimuth, maxElev, tran), spherToEuc(MaxRange, minAzimuth, maxElev, tran), FColor::Green, false, .03*TicksPerCapture, ECC_WorldStatic, 1.f);
			DrawDebugLine(GetWorld(), spherToEuc(MinRange, maxAzimuth, minElev, tran), spherToEuc(MaxRange, maxAzimuth, minElev, tran), FColor::Green, false, .03*TicksPerCapture, ECC_WorldStatic, 1.f);
			DrawDebugLine(GetWorld(), spherToEuc(MinRange, maxAzimuth, maxElev, tran), spherToEuc(MaxRange, maxAzimuth, maxElev, tran), FColor::Green, false, .03*TicksPerCapture, ECC_WorldStatic, 1.f);

			// azimuth lines (should be arcs, we're being lazy)
			DrawDebugLine(GetWorld(), spherToEuc(MinRange, minAzimuth, minElev, tran), spherToEuc(MinRange, maxAzimuth, minElev, tran), FColor::Green, false, .03*TicksPerCapture, ECC_WorldStatic, 1.f);
			DrawDebugLine(GetWorld(), spherToEuc(MinRange, minAzimuth, maxElev, tran), spherToEuc(MinRange, maxAzimuth, maxElev, tran), FColor::Green, false, .03*TicksPerCapture, ECC_WorldStatic, 1.f);
			DrawDebugLine(GetWorld(), spherToEuc(MaxRange, minAzimuth, minElev, tran), spherToEuc(MaxRange, maxAzimuth, minElev, tran), FColor::Green, false, .03*TicksPerCapture, ECC_WorldStatic, 1.f);
			DrawDebugLine(GetWorld(), spherToEuc(MaxRange, minAzimuth, maxElev, tran), spherToEuc(MaxRange, maxAzimuth, maxElev, tran), FColor::Green, false, .03*TicksPerCapture, ECC_WorldStatic, 1.f);

			// elevation lines (should be arcs, we're being lazy)
			DrawDebugLine(GetWorld(), spherToEuc(MinRange, minAzimuth, minElev, tran), spherToEuc(MinRange, minAzimuth, maxElev, tran), FColor::Green, false, .03*TicksPerCapture, ECC_WorldStatic, 1.f);
			DrawDebugLine(GetWorld(), spherToEuc(MinRange, maxAzimuth, minElev, tran), spherToEuc(MinRange, maxAzimuth, maxElev, tran), FColor::Green, false, .03*TicksPerCapture, ECC_WorldStatic, 1.f);
			DrawDebugLine(GetWorld(), spherToEuc(MaxRange, minAzimuth, minElev, tran), spherToEuc(MaxRange, minAzimuth, maxElev, tran), FColor::Green, false, .03*TicksPerCapture, ECC_WorldStatic, 1.f);
			DrawDebugLine(GetWorld(), spherToEuc(MaxRange, maxAzimuth, minElev, tran), spherToEuc(MaxRange, maxAzimuth, maxElev, tran), FColor::Green, false, .03*TicksPerCapture, ECC_WorldStatic, 1.f);
		}

		// Reset everything for next timestep
		TickCounter = 0;
		std::fill(count, count+BinsRange*BinsAzimuth, 0);
		leafs.Reset();
		for(auto& tl: tempLeafs){
			tl.Reset();
		}
	}
}
