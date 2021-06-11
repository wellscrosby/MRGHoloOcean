#include "Holodeck.h"
#include "OpticalModemSensor.h"
#include "Json.h"

UOpticalModemSensor::UOpticalModemSensor() {
    PrimaryComponentTick.bCanEverTick = true;
	SensorName = "OpticalModemSensor";
}

void UOpticalModemSensor::InitializeSensor() {
	Super::InitializeSensor();
	//You need to get the pointer to the object the sensor is attached to. 
	Parent = Cast<UPrimitiveComponent>(this->GetAttachParent());
}

void UOpticalModemSensor::TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	//check if your parent pointer is valid, and if the sensor is on. Then get the velocity and buffer, then send the data to it. 
    int* IntBuffer = static_cast<int*>(Buffer);
    if (Parent != nullptr && bOn) {
		// if someone starting transmitting
		if (fromSensor) {
            int* temp = this->CanTransmit();
			IntBuffer[0] = temp[0]; //Returns 1 or 0 for true and false respectively
            for (int i = 0; i < 3; i++) {
                IntBuffer[i+1] = temp[i];
    }
		}
        else {
            IntBuffer[0] = -2; //indicates no fromSensor
        }
	}
    else {
        IntBuffer[0] = -1; //indicates not on or no parent
    }
    

    if (LaserDebug) {
        DrawDebugCone(GetWorld(), GetComponentLocation(), GetForwardVector(), MaxDistance * 100, FMath::DegreesToRadians(LaserAngle), FMath::DegreesToRadians(LaserAngle), DebugNumSides, DebugColor, false, .01, ECC_WorldStatic, 1.F);
    }
}
	
int* UOpticalModemSensor::CanTransmit() {

    static int dataOut [3] = {1,1,1};

    // get coordinates of other sensor in local frame
    FTransform SensortoWorld = this->GetComponentTransform();
    FVector fromLocation = fromSensor->GetComponentLocation();
    FVector fromLocationLocal = UKismetMathLibrary::InverseTransformLocation(SensortoWorld, fromLocation);
    fromLocationLocal = ConvertLinearVector(fromLocationLocal, UEToClient);

    float dist = UKismetMathLibrary::Sqrt(FMath::Square(fromLocationLocal.X) + FMath::Square(fromLocationLocal.Y) + FMath::Square(fromLocationLocal.Z));
    UE_LOG(LogHolodeck, Log, TEXT("dist = %f  MaxDistance = %f"), dist, MaxDistance);

    //Max guaranteed range of modem is 50 meters
    if (dist > MaxDistance) {
        // return 0;
        dataOut[0] = 0;
    }
    else {
        FTransform FromSensortoWorld = fromSensor->GetComponentTransform();
        FVector toLocation = this->GetComponentLocation();
        FVector fromSensorLocal = UKismetMathLibrary::InverseTransformLocation(FromSensortoWorld, toLocation);
        fromSensorLocal = ConvertLinearVector(fromLocationLocal, UEToClient);
        // Calculate if sensors are facing each other within 120 degrees
        //--> Difference in angle needs to be -60 < x < 60 
        //--> Check both sensors to make sure both are in acceptable orientations

        if (IsSensorOriented(fromLocationLocal) && IsSensorOriented(fromSensorLocal)) {
            // Calculate if rangefinder and dist are equal or not.
            FVector start = GetComponentLocation();

            FVector end = fromLocation;
            
            end = end * MaxDistance;
            end = start + end;

            FCollisionQueryParams QueryParams = FCollisionQueryParams();
            QueryParams.AddIgnoredActor(Cast<AActor>(Parent));

            FHitResult Hit = FHitResult();

            bool TraceResult = GetWorld()->LineTraceSingleByChannel(Hit, start, end, ECollisionChannel::ECC_Visibility, QueryParams);
            
            float range = (TraceResult ? Hit.Distance / 100 : 50);  // centimeter to meters
            UE_LOG(LogHolodeck, Log, TEXT("range = %f"),range);

            if (dist == range) {
                // return 1;
                dataOut[2] = 2;
            }
            else {
                // return 0;
                dataOut[2] = 0;
            }
        }
        else {
            // return 0;
            dataOut[1] = 0;
        }
    }

    return dataOut;
}


bool UOpticalModemSensor::IsSensorOriented(FVector localToSensor) {
    if (90 - LaserAngle <= FMath::RadiansToDegrees(UKismetMathLibrary::Atan2(localToSensor.X, localToSensor.Y)) && 
      FMath::RadiansToDegrees(UKismetMathLibrary::Atan2(localToSensor.X, localToSensor.Y)) <= 90 + LaserAngle) {

        if (90 - LaserAngle <= FMath::RadiansToDegrees(UKismetMathLibrary::Atan2(localToSensor.X, localToSensor.Z)) && 
          FMath::RadiansToDegrees(UKismetMathLibrary::Atan2(localToSensor.X, localToSensor.Z))<= 90 + LaserAngle) {

            if (90 - LaserAngle <= FMath::RadiansToDegrees(UKismetMathLibrary::Atan2(localToSensor.Y, localToSensor.Z)) && 
              FMath::RadiansToDegrees(UKismetMathLibrary::Atan2(localToSensor.Y, localToSensor.Z))<= 90 + LaserAngle) {
                return true;
            }
            else {
                return false;
            }
        }
        else {
            return false;
        }
    }
    else {
        return false;
    }
}

void UOpticalModemSensor::ParseSensorParms(FString ParmsJson) {
	Super::ParseSensorParms(ParmsJson);

	TSharedPtr<FJsonObject> JsonParsed;
	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(ParmsJson);
	if (FJsonSerializer::Deserialize(JsonReader, JsonParsed)) {

		if (JsonParsed->HasTypedField<EJson::Number>("MaxDistance")) {
			MaxDistance = JsonParsed->GetIntegerField("MaxDistance");
		}

		if (JsonParsed->HasTypedField<EJson::Number>("LaserAngle")) {
			LaserAngle = JsonParsed->GetIntegerField("LaserAngle");
		}

		if (JsonParsed->HasTypedField<EJson::Number>("DebugNumSides")) {
			DebugNumSides = JsonParsed->GetIntegerField("DebugNumSides");
		}

		if (JsonParsed->HasTypedField<EJson::Boolean>("LaserDebug")) {
			LaserDebug = JsonParsed->GetBoolField("LaserDebug");
		}
        if (JsonParsed->HasTypedField<EJson::String>("DebugColor")) {
            FillColorMap();
			DebugColor = ColorMap[JsonParsed->GetStringField("DebugColor")];
		}
	}
	else {
		UE_LOG(LogHolodeck, Fatal, TEXT("URangeFinderSensor::ParseSensorParms:: Unable to parse json."));
	}
}

void UOpticalModemSensor::FillColorMap() {
    ColorMap.Add("Black", FColor::Black);
    ColorMap.Add("Blue", FColor::Blue);
    ColorMap.Add("Cyan", FColor::Cyan);
    ColorMap.Add("Emerald", FColor::Emerald);
    ColorMap.Add("Green", FColor::Green);
    ColorMap.Add("Magenta", FColor::Magenta);
    ColorMap.Add("Orange", FColor::Orange);
    ColorMap.Add("Purple", FColor::Purple);
    ColorMap.Add("Red", FColor::Red);
    ColorMap.Add("Silver", FColor::Silver);
    ColorMap.Add("Transparent", FColor::Transparent);
    ColorMap.Add("Turquoise", FColor::Turquoise);
    ColorMap.Add("White", FColor::White);
    ColorMap.Add("Yellow", FColor::Yellow);
    ColorMap.Add("Random", FColor::MakeRandomColor());
}