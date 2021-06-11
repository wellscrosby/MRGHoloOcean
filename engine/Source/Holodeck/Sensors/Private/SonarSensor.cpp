// MIT License (c) 2019 BYU PCCL see LICENSE file

#include "Holodeck.h"
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
	}
}

// Allows sensor parameters to be set programmatically from client.
void USonarSensor::ParseSensorParms(FString ParmsJson) {
	Super::ParseSensorParms(ParmsJson);

	TSharedPtr<FJsonObject> JsonParsed;
	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(ParmsJson);
	if (FJsonSerializer::Deserialize(JsonReader, JsonParsed)) {

		if (JsonParsed->HasTypedField<EJson::Number>("MaxRange")) {
			MaxRange = JsonParsed->GetIntegerField("MaxRange")*100;
		}

		if (JsonParsed->HasTypedField<EJson::Number>("MinRange")) {
			MinRange = JsonParsed->GetIntegerField("MinRange")*100;
		}

		if (JsonParsed->HasTypedField<EJson::Number>("Azimuth")) {
			Azimuth = JsonParsed->GetIntegerField("Azimuth");
		}

		if (JsonParsed->HasTypedField<EJson::Number>("Elevation")) {
			Elevation = JsonParsed->GetIntegerField("Elevation");
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
}

bool USonarSensor::inRange(Octree* tree){
	FTransform SensortoWorld = this->GetComponentTransform();
	FVector locLocal = UKismetMathLibrary::InverseTransformLocation(SensortoWorld, tree->loc);

	tree->locSpherical.X = locLocal.Size();
	// UnitCartesianToSpherical?
	tree->locSpherical.Y = UKismetMathLibrary::DegAtan2(-locLocal.Y, locLocal.X);
	tree->locSpherical.Z = UKismetMathLibrary::DegAtan2(FVector2D(locLocal.X, locLocal.Y).Size(), locLocal.Z);

	// if it's a leaf, make sure it's exactly in
	if(tree->leafs.Num() == 0){
		// save impact normal that we happen to compute here for later
		tree->normalImpact = locLocal / tree->locSpherical.X;

		return ( MinRange < tree->locSpherical.X && tree->locSpherical.X < MaxRange &&
				minAzimuth < tree->locSpherical.Y && tree->locSpherical.Y < maxAzimuth &&
				minElev < tree->locSpherical.Z && tree->locSpherical.Z < maxElev);
	}
	// if it's not a leaf, give it some leeway, there may be a leaf in it that's still in
	else{
		return ( tree->locSpherical.X < MaxRange*1.1 &&
				-90 < tree->locSpherical.Y && tree->locSpherical.Y < 90 &&
				0 < tree->locSpherical.Z && tree->locSpherical.Z < 180);
	}
}	

void USonarSensor::leafsInRange(Octree* tree, TArray<Octree*>& leafs){
	bool in = inRange(tree);
	if(in){
		if(tree->leafs.Num() == 0){
			leafs.Add(tree);
		}
		else{
			for(Octree* l : tree->leafs){
				leafsInRange(l, leafs);
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
	float* FloatBuffer = static_cast<float*>(Buffer);
	TArray<Octree*> leafs;

	// FILTER TO GET THE LEAFS WE WANT
	for( Octree* t : octree){
		leafsInRange(t, leafs);
	}

	// SORT THEM & RUN CALCULATIONS
	TArray<int32> count;
	count.Init(0, BinsRange*BinsAzimuth);
	TArray<float> result;
	result.Init(0, BinsRange*BinsAzimuth);
	for( Octree* l : leafs){
		// find what bin they must go in
		int32 rBin = (int)(l->locSpherical[0] / RangeRes);
		int32 aBin = (int)(l->locSpherical[1] / AzimuthRes);
		int32 idx = rBin*BinsAzimuth + aBin;

		// compute contribution
		float val = FVector::DotProduct(l->normal, l->normalImpact);
		if(val > 0){
			// TODO: use sigmoid here?
			result[idx] += val;
			count[idx]++;
		}
	}

	// TODO: Introduce noise?

	// MOVE THEM INTO BUFFER
	for (int i = 0; i < BinsRange*BinsAzimuth; i++) {
		if(count[i] != 0){
			FloatBuffer[i] = result[i] / count[i];
		}
	}

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
