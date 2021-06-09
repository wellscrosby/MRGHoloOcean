// MIT License (c) 2019 BYU PCCL see LICENSE file

#include "Holodeck.h"
#include "SonarSensor.h"

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
			OctreeMax = JsonParsed->GetIntegerField("OctreeMax");
		}

		if (JsonParsed->HasTypedField<EJson::Number>("OctreeMin")) {
			OctreeMin = JsonParsed->GetIntegerField("OctreeMin");
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
}

void USonarSensor::TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {

	float* FloatBuffer = static_cast<float*>(Buffer);

	for (int i = 0; i < BinsRange*BinsAzimuth; i++) {
		
		FloatBuffer[i] = 1;

	}
}
