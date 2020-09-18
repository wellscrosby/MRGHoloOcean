#include "Holodeck.h"
#include "DVLSensor.h"

UDVLSensor::UDVLSensor() {
	PrimaryComponentTick.bCanEverTick = true;
	SensorName = "DVLSensor";
}

void UDVLSensor::InitializeSensor() {
	Super::InitializeSensor();

	//You need to get the pointer to the object the sensor is attached to. 
	Parent = this->GetAttachmentRootActor();
}

void UDVLSensor::TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	//check if your parent pointer is valid, and if the sensor is on. Then get the velocity and buffer, then send the data to it. 
	if (Parent != nullptr && bOn) {
		FVector Velocity = Parent->GetVelocity();
		Velocity = ConvertLinearVector(Velocity, UEToClient);
		float* FloatBuffer = static_cast<float*>(Buffer);
		FloatBuffer[0] = Velocity.X;
		FloatBuffer[1] = Velocity.Y;
	}
}
