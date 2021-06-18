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
        DrawDebugLine(GetWorld(), GetComponentLocation(), GetForwardVector() * MaxDistance * 100, DebugColor,false, .01,ECC_WorldStatic, 1.F);
    }
}
	
int* UOpticalModemSensor::CanTransmit() {

    static int dataOut [3] = {1,1,1};

    // get coordinates of other sensor in local frame
    FVector sendingSensor = this->GetComponentLocation();
    FVector receiveSensor = fromSensor->GetComponentLocation();
    FVector sendToReceive = receiveSensor - sendingSensor;
    FVector receiveToSend = sendingSensor - receiveSensor;

    float dist = sendToReceive.Size() / 100;
    UE_LOG(LogHolodeck, Log, TEXT("dist = %f  MaxDistance = %f"), dist, MaxDistance);
    UE_LOG(LogHolodeck, Log, TEXT("SensorLocation = %s  ParentLocation = %s"), *sendingSensor.ToString(), *Parent->GetComponentLocation().ToString())

    //Max guaranteed range of modem is 50 meters
    if (dist > MaxDistance) {
        // return 0;
        dataOut[0] = 0;
    }
    else {
        // Calculate if sensors are facing each other within 120 degrees
        //--> Difference in angle needs to be -60 < x < 60 
        //--> Check both sensors to make sure both are in acceptable orientations

        if (IsSensorOriented(this, sendToReceive) && IsSensorOriented(fromSensor, receiveToSend)) {
            // Calculate if rangefinder and dist are equal or not.

            FCollisionQueryParams QueryParams = FCollisionQueryParams();
            QueryParams.AddIgnoredComponent(Parent);

            FHitResult Hit = FHitResult();

            bool TraceResult = GetWorld()->LineTraceSingleByChannel(Hit, sendingSensor, receiveSensor, ECollisionChannel::ECC_Visibility, QueryParams);
           
            float range = (TraceResult && Hit.GetComponent() == fromSensor->Parent ? dist : Hit.Distance);
            UE_LOG(LogHolodeck, Log, TEXT("range = %f  object = %s"), range, *Hit.GetActor()->GetName());
            
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


bool UOpticalModemSensor::IsSensorOriented(UOpticalModemSensor* Sensor, FVector localToSensor) {
    float angle = FMath::RadiansToDegrees(UKismetMathLibrary::Acos(UKismetMathLibrary::Dot_VectorVector(UKismetMathLibrary::Normal(Sensor->GetForwardVector()), UKismetMathLibrary::Normal(localToSensor))));
    UE_LOG(LogHolodeck, Log, TEXT("angle = %f"),angle);

    if (-1 *LaserAngle < angle && angle < LaserAngle) {
        return true;
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