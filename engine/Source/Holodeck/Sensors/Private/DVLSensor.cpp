// MIT License (c) 2021 BYU FRoStLab see LICENSE file

#include "Holodeck.h"
#include "DVLSensor.h"

UDVLSensor::UDVLSensor() {
	PrimaryComponentTick.bCanEverTick = true;
	SensorName = "DVLSensor";
}

void UDVLSensor::ParseSensorParms(FString ParmsJson) {
	Super::ParseSensorParms(ParmsJson);

	TSharedPtr<FJsonObject> JsonParsed;
	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(ParmsJson);
	if (FJsonSerializer::Deserialize(JsonReader, JsonParsed)) {

		if (JsonParsed->HasTypedField<EJson::Number>("Elevation")) {
			elevation = JsonParsed->GetNumberField("Elevation");
		}
		if (JsonParsed->HasTypedField<EJson::Boolean>("DebugLines")) {
			DebugLines = JsonParsed->GetBoolField("DebugLines");
		}

		// For handling noise
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

	}
	else {
		UE_LOG(LogHolodeck, Fatal, TEXT("UDVLSensor::ParseSensorParms:: Unable to parse json."));
	}
}

void UDVLSensor::InitializeSensor() {
	Super::InitializeSensor();

	//You need to get the pointer to the object the sensor is attached to. 
	Parent = Cast<UPrimitiveComponent>(this->GetAttachParent());

	if(mvn.isUncertain() || DebugLines){
		sinElev = UKismetMathLibrary::DegSin(elevation);
		cosElev = UKismetMathLibrary::DegCos(elevation);
	}
	if(mvn.isUncertain()){
		// make transformation matrix
		// See https://etda.libraries.psu.edu/files/final_submissions/17327
		transform = {   {1/(2*sinElev),             0, -1/(2*sinElev),             0},
						{            0, 1/(2*sinElev),              0, -1/(2*sinElev)},
						{1/(4*cosElev), 1/(4*cosElev),  1/(4*cosElev),  1/(4*cosElev)} };
	}
	if(DebugLines){
		// make direction lines
		directions = {FVector(sinElev, 0, -cosElev),
					FVector(0, -sinElev, -cosElev),
					FVector(-sinElev, 0, -cosElev),
					FVector(0, sinElev, -cosElev)};
		// scale up to a large number
		for(FVector& d : directions) d *= 1000;
	}
}

void UDVLSensor::TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	//check if your parent pointer is valid, and if the sensor is on. Then get the velocity and buffer, then send the data to it. 
	if (Parent != nullptr && bOn) {
		//UE4 gives us world velocity, rotate it to get local velocity
		FRotator R = this->GetComponentRotation();
		FVector Velocity = Parent->GetPhysicsLinearVelocityAtPoint(this->GetComponentLocation());
		Velocity = R.UnrotateVector(Velocity);
		Velocity = ConvertLinearVector(Velocity, UEToClient);

		// Add noise if it's been enabled
		if(mvn.isUncertain()){
			TArray<float> sample = mvn.sampleTArray();
			for(int i=0;i<4;i++){
				Velocity.X += transform[0][i]*sample[i];
				Velocity.Y += transform[1][i]*sample[i];
				Velocity.Z += transform[2][i]*sample[i];
			}
		}

		// Send to buffer
		float* FloatBuffer = static_cast<float*>(Buffer);
		FloatBuffer[0] = Velocity.X;
		FloatBuffer[1] = Velocity.Y;
		FloatBuffer[2] = Velocity.Z;

		// display debug lines if we want
		if(DebugLines){
			FVector start = this->GetComponentLocation();
			for(FVector v : directions){
				FVector end = R.RotateVector(v)+start;
				DrawDebugLine(GetWorld(), start, end, FColor::Green, false, .01, ECC_WorldStatic, 1.f);
			}
		}
	}
}
