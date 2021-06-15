// MIT License (c) 2019 BYU PCCL see LICENSE file

#include "Holodeck.h"
#include "Benchmarker.h"
#include "SonarSensor.h"

TArray<Octree*> USonarSensor::octree;
FVector USonarSensor::EnvMin = FVector(0);
FVector USonarSensor::EnvMax = FVector(0);

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
}

// Allows sensor parameters to be set programmatically from client.
void USonarSensor::ParseSensorParms(FString ParmsJson) {
	Super::ParseSensorParms(ParmsJson);

	TSharedPtr<FJsonObject> JsonParsed;
	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(ParmsJson);
	if (FJsonSerializer::Deserialize(JsonReader, JsonParsed)) {

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
			OctreeMax = JsonParsed->GetNumberField("OctreeMax")*100;
		}

		if (JsonParsed->HasTypedField<EJson::Number>("OctreeMin")) {
			OctreeMin = JsonParsed->GetNumberField("OctreeMin")*100;
		}

		if (JsonParsed->HasTypedField<EJson::Boolean>("ViewDebug")) {
			ViewDebug = JsonParsed->GetBoolField("ViewDebug");
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
		FString file_path = FPaths::ProjectDir() + "/" + GetWorld()->GetMapName();
		FFileManagerGeneric().MakeDirectory(*file_path);

		FString filename = file_path + "/octree_" + FString::SanitizeFloat(OctreeMax, 4) + "_" + FString::SanitizeFloat(OctreeMin, 4) + ".json";

		// Clean environment size
		FVector min = FVector(FGenericPlatformMath::Min(EnvMin.X, EnvMax.X), FGenericPlatformMath::Min(EnvMin.Y, EnvMax.Y), FGenericPlatformMath::Min(EnvMin.Z, EnvMax.Z));
		FVector max = FVector(FGenericPlatformMath::Max(EnvMin.X, EnvMax.X), FGenericPlatformMath::Max(EnvMin.Y, EnvMax.Y), FGenericPlatformMath::Max(EnvMin.Z, EnvMax.Z));
		EnvMin = min;
		EnvMax = max;

		// check if it's been made yet
		if(FPaths::FileExists(filename)){
			octree = Octree::fromJson(filename);
		}
		else{
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
			Octree::toJson(octree, filename);
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

	leafs.Reserve(10000);
}

bool USonarSensor::inRange(Octree* tree, float size){
	FTransform SensortoWorld = this->GetComponentTransform();
	// if it's not a leaf, we use a bigger search area
	float offset = 0;
	if(tree->leafs.Num() != 0){
		float radius = (size/2)*sqrt2;
		offset = radius/sinOffset;
		SensortoWorld.AddToTranslation( -this->GetForwardVector()*offset );
		offset += radius;
	}

	// transform to spherical coordinates
	FVector locLocal = SensortoWorld.InverseTransformPositionNoScale(tree->loc);

	// check if azimuth is in
	tree->locSpherical.Y = UKismetMathLibrary::DegAtan2(-locLocal.Y, locLocal.X);
	if(minAzimuth > tree->locSpherical.Y || tree->locSpherical.Y > maxAzimuth) return false;

	// check if it's in range
	tree->locSpherical.X = locLocal.Size();
	if(MinRange > tree->locSpherical.X || tree->locSpherical.X > MaxRange+offset) return false; 

	// check if elevation is in
	tree->locSpherical.Z = UKismetMathLibrary::DegAtan2(FVector2D(locLocal.X, locLocal.Y).Size(), locLocal.Z);
	if(minElev > tree->locSpherical.Z || tree->locSpherical.Z > maxElev) return false;
	
	// otherwise it's in!
	return true;
}	

void USonarSensor::leafsInRange(Octree* tree, TArray<Octree*>& leafs, float size){
	bool in = inRange(tree, size);
	if(in){
		if(tree->leafs.Num() == 0){
			leafs.Add(tree);
		}
		else{
			for(Octree* l : tree->leafs){
				leafsInRange(l, leafs, size/2);
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
	float* result = static_cast<float*>(Buffer);

	// FILTER TO GET THE LEAFS WE WANT
	Benchmarker time;
	time.Start();
	for( Octree* t : octree){
		leafsInRange(t, leafs, OctreeMax);
	}
	time.End();
	UE_LOG(LogHolodeck, Warning, TEXT("Sorting took \t %f ms"), time.CalcMs())

	// SORT THEM & RUN CALCULATIONS
	time.Start();
	TArray<int32> count;
	count.Init(0, BinsRange*BinsAzimuth);
	for( Octree* l : leafs){
		// find what bin they must go in
		int32 rBin = (int)((l->locSpherical[0] - MinRange) / RangeRes);
		int32 aBin = (int)((l->locSpherical[1] - minAzimuth)/ AzimuthRes);
		int32 idx = rBin*BinsAzimuth + aBin;

		// Compute impact normal
		FVector normalImpact = this->GetComponentLocation() - l->loc; 
		normalImpact /= normalImpact.Size();

		// compute contribution
		float val = FVector::DotProduct(l->normal, normalImpact);
		if(val > 0){
			// TODO: use sigmoid here?
			result[idx] += val;
			count.GetData()[idx]++;
		}
	}
	leafs.Reset();
	time.End();
	UE_LOG(LogHolodeck, Warning, TEXT("Computing took \t %f ms"), time.CalcMs());

	// TODO: Introduce noise?

	// MOVE THEM INTO BUFFER
	time.Start();
	// Tried ParallelFor here, slowed it down. Might be beneficial for larger things though?
	for (int i = 0; i < BinsRange*BinsAzimuth; i++) {
		if(count[i] != 0){
			result[i] /= count.GetData()[i];
		}
		else{
			result[i] = 0;
		}
	}
	time.End();
	UE_LOG(LogHolodeck, Warning, TEXT("Dividing took \t %f ms"), time.CalcMs());

	// draw outlines of our region
	if(ViewDebug){
		for( Octree* l : leafs){
			DrawDebugPoint(GetWorld(), l->loc, 4.f, FColor::Red, false, .1f);
		}
		FTransform tran = this->GetComponentTransform();
		
		// range lines
		DrawDebugLine(GetWorld(), spherToEuc(MinRange, minAzimuth, minElev, tran), spherToEuc(MaxRange, minAzimuth, minElev, tran), FColor::Green, false, .01, ECC_WorldStatic, 1.f);
		DrawDebugLine(GetWorld(), spherToEuc(MinRange, minAzimuth, maxElev, tran), spherToEuc(MaxRange, minAzimuth, maxElev, tran), FColor::Green, false, .01, ECC_WorldStatic, 1.f);
		DrawDebugLine(GetWorld(), spherToEuc(MinRange, maxAzimuth, minElev, tran), spherToEuc(MaxRange, maxAzimuth, minElev, tran), FColor::Green, false, .01, ECC_WorldStatic, 1.f);
		DrawDebugLine(GetWorld(), spherToEuc(MinRange, maxAzimuth, maxElev, tran), spherToEuc(MaxRange, maxAzimuth, maxElev, tran), FColor::Green, false, .01, ECC_WorldStatic, 1.f);

		// azimuth lines (should be arcs, we're being lazy)
		DrawDebugLine(GetWorld(), spherToEuc(MinRange, minAzimuth, minElev, tran), spherToEuc(MinRange, maxAzimuth, minElev, tran), FColor::Green, false, .01, ECC_WorldStatic, 1.f);
		DrawDebugLine(GetWorld(), spherToEuc(MinRange, minAzimuth, maxElev, tran), spherToEuc(MinRange, maxAzimuth, maxElev, tran), FColor::Green, false, .01, ECC_WorldStatic, 1.f);
		DrawDebugLine(GetWorld(), spherToEuc(MaxRange, minAzimuth, minElev, tran), spherToEuc(MaxRange, maxAzimuth, minElev, tran), FColor::Green, false, .01, ECC_WorldStatic, 1.f);
		DrawDebugLine(GetWorld(), spherToEuc(MaxRange, minAzimuth, maxElev, tran), spherToEuc(MaxRange, maxAzimuth, maxElev, tran), FColor::Green, false, .01, ECC_WorldStatic, 1.f);

		// elevation lines (should be arcs, we're being lazy)
		DrawDebugLine(GetWorld(), spherToEuc(MinRange, minAzimuth, minElev, tran), spherToEuc(MinRange, minAzimuth, maxElev, tran), FColor::Green, false, .01, ECC_WorldStatic, 1.f);
		DrawDebugLine(GetWorld(), spherToEuc(MinRange, maxAzimuth, minElev, tran), spherToEuc(MinRange, maxAzimuth, maxElev, tran), FColor::Green, false, .01, ECC_WorldStatic, 1.f);
		DrawDebugLine(GetWorld(), spherToEuc(MaxRange, minAzimuth, minElev, tran), spherToEuc(MaxRange, minAzimuth, maxElev, tran), FColor::Green, false, .01, ECC_WorldStatic, 1.f);
		DrawDebugLine(GetWorld(), spherToEuc(MaxRange, maxAzimuth, minElev, tran), spherToEuc(MaxRange, maxAzimuth, maxElev, tran), FColor::Green, false, .01, ECC_WorldStatic, 1.f);

	}
}
