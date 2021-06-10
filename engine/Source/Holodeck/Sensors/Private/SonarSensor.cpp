// MIT License (c) 2019 BYU PCCL see LICENSE file

#include "Holodeck.h"
#include "SonarSensor.h"

TArray<Octree*> USonarSensor::octree;
FVector USonarSensor::EnvMin = FVector(0);
FVector USonarSensor::EnvMax = FVector(0);

USonarSensor::USonarSensor() {
	SensorName = "SonarSensor";
}

// Allows sensor parameters to be set programmatically from client.
void USonarSensor::ParseSensorParms(FString ParmsJson) {
	Super::ParseSensorParms(ParmsJson);

	TSharedPtr<FJsonObject> JsonParsed;
	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(ParmsJson);
	if (FJsonSerializer::Deserialize(JsonReader, JsonParsed)) {

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
}

void USonarSensor::TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	float* FloatBuffer = static_cast<float*>(Buffer);

	for (int i = 0; i < BinsRange*BinsAzimuth; i++) {
		
		FloatBuffer[i] = 1;

	}
}
