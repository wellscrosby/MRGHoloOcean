// MIT License (c) 2021 BYU FRoStLab see LICENSE file

#include "Holodeck.h"
#include "AcousticBeaconSensor.h"

UAcousticBeaconSensor::UAcousticBeaconSensor() {
	PrimaryComponentTick.bCanEverTick = true;
	SensorName = "AcousticBeaconSensor";
}

void UAcousticBeaconSensor::InitializeSensor() {
	Super::InitializeSensor();

	//You need to get the pointer to the object the sensor is attached to. 
	Parent = this->GetAttachmentRootActor();
}

void UAcousticBeaconSensor::ParseSensorParms(FString ParmsJson) {
	Super::ParseSensorParms(ParmsJson);

	TSharedPtr<FJsonObject> JsonParsed;
	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(ParmsJson);
	if (FJsonSerializer::Deserialize(JsonReader, JsonParsed)) {

		if (JsonParsed->HasTypedField<EJson::Number>("MaxDistance")) {
			MaxDistance = JsonParsed->GetNumberField("MaxDistance");
		}

        if (JsonParsed->HasTypedField<EJson::Number>("DistanceSigma")) {
			DistanceNoise.initSigma(JsonParsed->GetNumberField("DistanceSigma"));
        }
		if (JsonParsed->HasTypedField<EJson::Number>("DistanceCov")) {
			DistanceNoise.initCov(JsonParsed->GetNumberField("DistanceCov"));
		}

		if (JsonParsed->HasTypedField<EJson::Boolean>("CheckVisible")) {
			CheckVisible = JsonParsed->GetBoolField("CheckVisible");
		}
	}
	else {
		UE_LOG(LogHolodeck, Fatal, TEXT("UAcousticBeaconSensor::ParseSensorParms:: Unable to parse json."));
	}
}

void UAcousticBeaconSensor::TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	//check if your parent pointer is valid, and if the sensor is on. Then get the velocity and buffer, then send the data to it. 
	if (Parent != nullptr && bOn) {
		// if someone starting transmitting
		if (fromSensor){
			// get coordinates of other sensor in local frame
			FVector toLocation = this->GetComponentLocation();
			FTransform SensortoWorld = this->GetComponentTransform();
			FVector fromLocation = fromSensor->GetComponentLocation();
			FVector fromLocationLocal = UKismetMathLibrary::InverseTransformLocation(SensortoWorld, fromLocation);
			fromLocationLocal = ConvertLinearVector(fromLocationLocal, UEToClient);

			bool inRange = true;
			bool canSee = true;

			// Check line of sight if it's enabled
			if(fromSensor->CheckVisible){
				FCollisionQueryParams QueryParams = FCollisionQueryParams();
				QueryParams.AddIgnoredActor(fromSensor->Parent);
				FHitResult Hit = FHitResult();
				bool TraceResult = GetWorld()->LineTraceSingleByChannel(Hit, fromLocation, toLocation, ECollisionChannel::ECC_Visibility, QueryParams);
			
				// Can see if we hit something and it's the vehicle that sent it
				canSee = (TraceResult && Hit.GetActor() == Parent);
			}

			// Check distance if it's enabled
			if(fromSensor->MaxDistance > 0){
				float Max = fromSensor->MaxDistance + fromSensor->DistanceNoise.sampleFloat();
				inRange = fromLocationLocal.Size() < Max;
			}

			// calculate angles
			WaitBuffer[0] = UKismetMathLibrary::Atan2(fromLocationLocal.Y, fromLocationLocal.X);
			float dist = UKismetMathLibrary::Sqrt( FMath::Square(fromLocationLocal.X) + FMath::Square(fromLocationLocal.Y) );
			WaitBuffer[1] = UKismetMathLibrary::Atan2(fromLocationLocal.Z, dist);

			// distance away (already converted)
			WaitBuffer[2] = fromLocationLocal.Size();

			// Z Value
			WaitBuffer[3] = ConvertUnrealDistanceToClient( toLocation.Z );

			// empty the sensor we got it from
			fromSensor = NULL;

			// figure out how long to receive that message
			WaitTicks = roundl(WaitBuffer[2] / (DeltaTime * SpeedOfSound));

			// If it can't be received, make buffer full of -1s to alert python side of failure
			if(!inRange || !canSee){
				for(int i=0; i<4; i++){
					WaitBuffer[i] = -1;
				}
			}
		}
		// if we're waiting for message
		if(WaitTicks > 0){
			WaitTicks--;
		}
		// if the message was received
		else if(WaitTicks == 0){
			float* FloatBuffer = static_cast<float*>(Buffer);
			for(int i=0;i<4;i++)
				FloatBuffer[i] = WaitBuffer[i];

			WaitTicks = -1;
		}
		// remove message after it was received
		else if(WaitTicks == -1){
			float* FloatBuffer = static_cast<float*>(Buffer);
			for(int i=0;i<4;i++)
				FloatBuffer[i] = std::numeric_limits<double>::quiet_NaN();

			WaitTicks = -2;
		}
	}
}
