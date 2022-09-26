// MIT License (c) 2021 BYU FRoStLab see LICENSE file

#include "Holodeck.h"
#include "MagnetometerSensor.h"

UMagnetometerSensor::UMagnetometerSensor() {
	PrimaryComponentTick.bCanEverTick = true;
	SensorName = "MagnetometerSensor";
}

void UMagnetometerSensor::ParseSensorParms(FString ParmsJson) {
	Super::ParseSensorParms(ParmsJson);

	TSharedPtr<FJsonObject> JsonParsed;
	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(ParmsJson);
	if (FJsonSerializer::Deserialize(JsonReader, JsonParsed)) {

		if (JsonParsed->HasTypedField<EJson::Number>("Sigma")) {
			mvn.initSigma(JsonParsed->GetNumberField("Sigma"));
		}
		if (JsonParsed->HasTypedField<EJson::Array>("Sigma")) {
			mvn.initSigma(JsonParsed->GetArrayField("Sigma"));
		}

		if (JsonParsed->HasTypedField<EJson::Number>("Cov")) {
			mvn.initCov(JsonParsed->GetNumberField("Cov"));
		}
		if (JsonParsed->HasTypedField<EJson::Array>("Cov")) {
			mvn.initCov(JsonParsed->GetArrayField("Cov"));
		}


		if (JsonParsed->HasTypedField<EJson::Array>("MagneticVector")) {
			TArray<TSharedPtr<FJsonValue>> b = JsonParsed->GetArrayField("MagneticVector");

			if(b.Num() == 3){
				MeasuredVector[0] = b[0]->AsNumber();
				MeasuredVector[1] = -1*b[1]->AsNumber(); // Switch to left handed
				MeasuredVector[2] = b[2]->AsNumber();
			}
			else{
				UE_LOG(LogHolodeck, Fatal, TEXT("UMagnetometerSensor::ParseSensorParms:: MagneticVector had wrong size."));
			}
		}

	
	}
	else {
		UE_LOG(LogHolodeck, Fatal, TEXT("UMagnetometerSensor::ParseSensorParms:: Unable to parse json."));
	}
}

void UMagnetometerSensor::InitializeSensor() {
	Super::InitializeSensor();

	//You need to get the pointer to the object you are attached to. 
	Parent = this->GetAttachParent();
}

void UMagnetometerSensor::TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	//check if your parent pointer is valid, and if the sensor is on. Then get the location and buffer, then send the location to the buffer. 
	if (Parent != nullptr && bOn) {
		float* FloatBuffer = static_cast<float*>(Buffer);

		FRotator Rotation = this->GetComponentRotation();
		FVector measurement = Rotation.UnrotateVector(MeasuredVector);
		measurement += mvn.sampleFVector();

		FloatBuffer[0] = measurement.X;
		FloatBuffer[1] = -1*measurement.Y; // Switch to right handed
		FloatBuffer[2] = measurement.Z;
	}
}
