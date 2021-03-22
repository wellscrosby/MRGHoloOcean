#include "Holodeck.h"
#include "AcousticBeaconSensor.h"

UAcousticBeaconSensor::UAcousticBeaconSensor() {
	PrimaryComponentTick.bCanEverTick = true;
	SensorName = "AcousticBeaconSensor";
}

void UAcousticBeaconSensor::InitializeSensor() {
	Super::InitializeSensor();

	//You need to get the pointer to the object the sensor is attached to. 
	Parent = Cast<UPrimitiveComponent>(this->GetAttachParent());
}

void UAcousticBeaconSensor::TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	//check if your parent pointer is valid, and if the sensor is on. Then get the velocity and buffer, then send the data to it. 
	if (Parent != nullptr && bOn) {
		float* FloatBuffer = static_cast<float*>(Buffer);
		FloatBuffer[0] = num;
		FloatBuffer[1] = 11.0;
	}
}
