// MIT License (c) 2019 BYU PCCL see LICENSE file

#include "Holodeck.h"
#include "Benchmarker.h"
#include "SonarSensor.h"

TArray<Octree*> USonarSensor::octree;
FVector USonarSensor::EnvMin = FVector(0);
FVector USonarSensor::EnvMax = FVector(0);

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

	// TODO: This may cause issues if we remove a sonar, but are still using others
	// unlikely, but could happen
	if(octree.Num() != 0){
		for(Octree* t : octree){
			delete t;
		}
		octree.Empty();
	}
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

		if (JsonParsed->HasTypedField<EJson::Number>("OctreeMax")) {
			OctreeMax = (int) (JsonParsed->GetNumberField("OctreeMax")*100);
		}

		if (JsonParsed->HasTypedField<EJson::Number>("OctreeMin")) {
			OctreeMin = (int) (JsonParsed->GetNumberField("OctreeMin")*100);
		}

		if (JsonParsed->HasTypedField<EJson::Boolean>("ViewDebug")) {
			ViewDebug = JsonParsed->GetBoolField("ViewDebug");
		}

		if (JsonParsed->HasTypedField<EJson::Number>("TicksPerCapture")) {
			TicksPerCapture = JsonParsed->GetIntegerField("TicksPerCapture");
		}

		// TODO: Move Environment size to environment settings instead of in sonar settings
		if (JsonParsed->HasTypedField<EJson::Array>("EnvMin")) {
			if(EnvMin == FVector(0)){
				TArray<TSharedPtr<FJsonValue>> min = JsonParsed->GetArrayField("EnvMin");
				EnvMin = ConvertLinearVector(FVector(min[0]->AsNumber(), min[1]->AsNumber(), min[2]->AsNumber()), ClientToUE);
			}
			else{
				UE_LOG(LogHolodeck, Warning, TEXT("You set EnvMin multiple times, this causes inconsistent behavior."));
			}
		}

		if (JsonParsed->HasTypedField<EJson::Array>("EnvMax")) {
			if(EnvMax == FVector(0)){
				TArray<TSharedPtr<FJsonValue>> max = JsonParsed->GetArrayField("EnvMax");
				EnvMax = ConvertLinearVector(FVector(max[0]->AsNumber(), max[1]->AsNumber(), max[2]->AsNumber()), ClientToUE);
			}
			else{
				UE_LOG(LogHolodeck, Warning, TEXT("You set EnvMax multiple times, this causes inconsistent behavior."));
			}
		}
	}
	else {
		UE_LOG(LogHolodeck, Fatal, TEXT("USonarSensor::ParseSensorParms:: Unable to parse json."));
	}
}

void USonarSensor::InitializeSensor() {
	Super::InitializeSensor();

	//You need to get the pointer to the object you are attached to. 
	Parent = this->GetAttachmentRootActor();
	Octree::ignoreActor(Parent);

	// Initialize octree if it hasn't been yet
	// TODO: We may start this before adding in all actors to be ignored
	if(octree.Num() == 0){
		// Clean up max size so it's a multiple fo minSize
		float temp = OctreeMin;
		while(temp <= OctreeMax){
			temp *= 2;
		}
		OctreeMax = temp;

		FString filePath = FPaths::ProjectDir() + "Octrees/" + GetWorld()->GetMapName();
		filePath += "/" + FString::FromInt(OctreeMin) + "_" + FString::FromInt(OctreeMax);

		// Clean environment size
		FVector min = FVector((int)FGenericPlatformMath::Min(EnvMin.X, EnvMax.X), (int)FGenericPlatformMath::Min(EnvMin.Y, EnvMax.Y), (int)FGenericPlatformMath::Min(EnvMin.Z, EnvMax.Z));
		FVector max = FVector((int)FGenericPlatformMath::Max(EnvMin.X, EnvMax.X), (int)FGenericPlatformMath::Max(EnvMin.Y, EnvMax.Y), (int)FGenericPlatformMath::Max(EnvMin.Z, EnvMax.Z));
		EnvMin = min;
		EnvMax = max;

		// check if it's been made yet
		if(FPaths::DirectoryExists(filePath)){
			UE_LOG(LogHolodeck, Warning, TEXT("SonarSensor::Loading Octree.."));
			octree = Octree::fromJson(filePath);
		}
		else{
			UE_LOG(LogHolodeck, Warning, TEXT("SonarSensor::Making Octree.."));
			// Otherwise, make the octrees
			FVector nCells = (EnvMax - EnvMin) / OctreeMax;
			for(int i = 0; i < nCells.X; i++) {
				for(int j = 0; j < nCells.Y; j++) {
					for(int k = 0; k < nCells.Z; k++) {
						FVector center = FVector(i*OctreeMax, j*OctreeMax, k*OctreeMax) + EnvMin;
						Octree::makeOctree(center, OctreeMax, GetWorld(), octree, OctreeMin);
					}
				}
			}
			// save for next time
			UE_LOG(LogHolodeck, Warning, TEXT("SonarSensor::Saving Octree.. to %s"), *filePath);
			Octree::toJson(octree, filePath);
		}
	}

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
	leafs.Reserve(10000);
	tempLeafs.Reserve(octree.Num());
	for(int i=0;i<octree.Num();i++){
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

void USonarSensor::TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	TickCounter++;
	if(TickCounter == TicksPerCapture){
		float* result = static_cast<float*>(Buffer);
		std::fill(result, result+BinsRange*BinsAzimuth, 0);

		// FILTER TO GET THE LEAFS WE WANT
		// Benchmarker time;
		// time.Start();
		ParallelFor(octree.Num(), [&](int32 i){
			leafsInRange(octree.GetData()[i], tempLeafs.GetData()[i], OctreeMax);
		});
		for(auto& tl : tempLeafs){
			leafs += tl;
		}
		// time.End();
		// UE_LOG(LogHolodeck, Warning, TEXT("PARALLEL, RAW Sort took \t %f ms"), time.CalcMs())

		// SORT THEM & RUN CALCULATIONS
		// time.Start();
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
		// time.End();
		// UE_LOG(LogHolodeck, Warning, TEXT("Computing took \t %f ms"), time.CalcMs())
		
		// MOVE THEM INTO BUFFER
		// time.Start();
		for (int i = 0; i < BinsRange*BinsAzimuth; i++) {
			if(count[i] != 0){
				result[i] *= (1 + multNoise.sampleFloat())/count[i];
				result[i] += addNoise.sampleRayleigh();
			}
			else{
				result[i] = addNoise.sampleRayleigh();
			}
		}
		// time.End();
		// UE_LOG(LogHolodeck, Warning, TEXT("Dividing took \t %f ms"), time.CalcMs());
		// UE_LOG(LogHolodeck, Warning, TEXT("Total Leafs: \t %d"), leafs.Num());

		// draw outlines of our region
		if(ViewDebug){
			for( Octree* l : leafs){
				DrawDebugPoint(GetWorld(), l->loc, .01*TicksPerCapture, FColor::Red, false, .1f);
			}
			FTransform tran = this->GetComponentTransform();
			
			// range lines
			DrawDebugLine(GetWorld(), spherToEuc(MinRange, minAzimuth, minElev, tran), spherToEuc(MaxRange, minAzimuth, minElev, tran), FColor::Green, false, .01*TicksPerCapture, ECC_WorldStatic, 1.f);
			DrawDebugLine(GetWorld(), spherToEuc(MinRange, minAzimuth, maxElev, tran), spherToEuc(MaxRange, minAzimuth, maxElev, tran), FColor::Green, false, .01*TicksPerCapture, ECC_WorldStatic, 1.f);
			DrawDebugLine(GetWorld(), spherToEuc(MinRange, maxAzimuth, minElev, tran), spherToEuc(MaxRange, maxAzimuth, minElev, tran), FColor::Green, false, .01*TicksPerCapture, ECC_WorldStatic, 1.f);
			DrawDebugLine(GetWorld(), spherToEuc(MinRange, maxAzimuth, maxElev, tran), spherToEuc(MaxRange, maxAzimuth, maxElev, tran), FColor::Green, false, .01*TicksPerCapture, ECC_WorldStatic, 1.f);

			// azimuth lines (should be arcs, we're being lazy)
			DrawDebugLine(GetWorld(), spherToEuc(MinRange, minAzimuth, minElev, tran), spherToEuc(MinRange, maxAzimuth, minElev, tran), FColor::Green, false, .01*TicksPerCapture, ECC_WorldStatic, 1.f);
			DrawDebugLine(GetWorld(), spherToEuc(MinRange, minAzimuth, maxElev, tran), spherToEuc(MinRange, maxAzimuth, maxElev, tran), FColor::Green, false, .01*TicksPerCapture, ECC_WorldStatic, 1.f);
			DrawDebugLine(GetWorld(), spherToEuc(MaxRange, minAzimuth, minElev, tran), spherToEuc(MaxRange, maxAzimuth, minElev, tran), FColor::Green, false, .01*TicksPerCapture, ECC_WorldStatic, 1.f);
			DrawDebugLine(GetWorld(), spherToEuc(MaxRange, minAzimuth, maxElev, tran), spherToEuc(MaxRange, maxAzimuth, maxElev, tran), FColor::Green, false, .01*TicksPerCapture, ECC_WorldStatic, 1.f);

			// elevation lines (should be arcs, we're being lazy)
			DrawDebugLine(GetWorld(), spherToEuc(MinRange, minAzimuth, minElev, tran), spherToEuc(MinRange, minAzimuth, maxElev, tran), FColor::Green, false, .01*TicksPerCapture, ECC_WorldStatic, 1.f);
			DrawDebugLine(GetWorld(), spherToEuc(MinRange, maxAzimuth, minElev, tran), spherToEuc(MinRange, maxAzimuth, maxElev, tran), FColor::Green, false, .01*TicksPerCapture, ECC_WorldStatic, 1.f);
			DrawDebugLine(GetWorld(), spherToEuc(MaxRange, minAzimuth, minElev, tran), spherToEuc(MaxRange, minAzimuth, maxElev, tran), FColor::Green, false, .01*TicksPerCapture, ECC_WorldStatic, 1.f);
			DrawDebugLine(GetWorld(), spherToEuc(MaxRange, maxAzimuth, minElev, tran), spherToEuc(MaxRange, maxAzimuth, maxElev, tran), FColor::Green, false, .01*TicksPerCapture, ECC_WorldStatic, 1.f);
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
