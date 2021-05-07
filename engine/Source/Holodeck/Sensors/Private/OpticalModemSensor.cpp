#include "Holodeck.h"
#include "OpticalModemSensor.h"

UOpticalModemSensor::UOpticalModemSensor() {
    PrimaryComponentTick.bCanEverTick = true;
	SensorName = "OpticalModemSensor";
}

void UOpticalModemSensor::InitializeSensor() {
	Super::InitializeSensor();

	//You need to get the pointer to the object the sensor is attached to. 
	Parent = Cast<UPrimitiveComponent>(this->GetAttachParent());
}

bool UOpticalModemSensor::CanTransmit(AActor recipient) {
    //check if your parent pointer is valid, and if the sensor is on. Then get the velocity and buffer, then send the data to it. 
	if (Parent != nullptr && bOn) {
		// if someone starting transmitting
		if (fromSensor) {
			// get coordinates of other sensor in local frame
			FTransform SensortoWorld = this->GetComponentTransform();
			FVector fromLocation = fromSensor->GetComponentLocation();
			FVector fromLocationLocal = UKismetMathLibrary::InverseTransformLocation(SensortoWorld, fromLocation);
			fromLocationLocal = ConvertLinearVector(fromLocationLocal, UEToClient);

            float dist = UKismetMathLibrary::Sqrt(Square(fromLocationLocal.X) + Square(fromLocationLocal.Y) + Square(fromLocationLocal.Z));

            //Max guaranteed range of modem is 50 meters
            if (dist > 50) {
                return false;
            }
            else {
                FVector fromSensorLocal = UKismetMathLibrary::InverseTransformLocation(fromLocation, SensortoWorld);
                fromSensorLocal = ConvertLinearVector(fromLocationLocal, UEToClient);
                // Calculate if sensors are facing each other within 120 degrees
                //--> Difference in angle needs to be -60 < x < 60 
                //--> Check both sensors to make sure both are in acceptable orientations

                if (IsSensorOriented(fromLocationLocal) && IsSensorOriented(fromSensorLocal)) {
                    // Calculate if rangefinder and dist are equal or not.
                    FVector start = GetComponentLocation();

                    FVector end = fromLocation;
                    
                    end = end * LaserMaxDistance;
                    end = start + end;

                    FCollisionQueryParams QueryParams = FCollisionQueryParams();
                    QueryParams.AddIgnoredActor(Parent);

                    FHitResult Hit = FHitResult();

                    bool TraceResult = GetWorld()->LineTraceSingleByChannel(Hit, start, end, ECollisionChannel::ECC_Visibility, QueryParams);
                    
                    float range = (TraceResult ? Hit.Distance / 100 : 50);  // centimeter to meters

                    if (dist == range) {
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

        }
    }
}

bool UOpticalModemSensor::IsSensorOriented(FVector localToSensor) {
    if (-60 <= FMath::RadiansToDegrees(UKismetMathLibrary::Atan2(localToSensor.X, localToSensor.Y)) && 
      FMath::RadiansToDegrees(UKismetMathLibrary::Atan2(localToSensor.X, localToSensor.Y))<= 60) {

        if (-60 <= FMath::RadiansToDegrees(UKismetMathLibrary::Atan2(localToSensor.X, localToSensor.Z)) && 
          FMath::RadiansToDegrees(UKismetMathLibrary::Atan2(localToSensor.X, localToSensor.Z))<= 60) {

            if (-60 <= FMath::RadiansToDegrees(UKismetMathLibrary::Atan2(localToSensor.Z, localToSensor.Y)) && 
              FMath::RadiansToDegrees(UKismetMathLibrary::Atan2(localToSensor.Z, localToSensor.Y))<= 60) {
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

