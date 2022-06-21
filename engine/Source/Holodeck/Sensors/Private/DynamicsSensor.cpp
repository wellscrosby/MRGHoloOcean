// MIT License (c) 2021 BYU FRoStLab see LICENSE file

#include "Holodeck.h"
#include "DynamicsSensor.h"

UDynamicsSensor::UDynamicsSensor() {
	PrimaryComponentTick.bCanEverTick = true;
	SensorName = "DynamicsSensor";
}

void UDynamicsSensor::ParseSensorParms(FString ParmsJson) {
	Super::ParseSensorParms(ParmsJson);

	TSharedPtr<FJsonObject> JsonParsed;
	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(ParmsJson);
	if (FJsonSerializer::Deserialize(JsonReader, JsonParsed)) {

		if (JsonParsed->HasTypedField<EJson::Boolean>("UseCOM")) {
			UseCOM = JsonParsed->GetBoolField("UseCOM");
		}

	}
	else {
		UE_LOG(LogHolodeck, Fatal, TEXT("UDynamicsSensor::ParseSensorParms:: Unable to parse json."));
	}
}

void UDynamicsSensor::InitializeSensor() {
	Super::InitializeSensor();

	//You need to get the pointer to the object you are attached to. 
	Parent = Cast<UPrimitiveComponent>(this->GetAttachParent());

	// Initialize all vectors
	LinearVelocityThen = FVector();
	AngularVelocityThen = FVector();

	LinearAcceleration = FVector();
	LinearVelocity = FVector();
	Position = FVector();
	AngularAcceleration = FVector();
	AngularVelocity = FVector();
	Rotation = FQuat();
}

void UDynamicsSensor::TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	//check if your parent pointer is valid, and if the sensor is on. Then get the location and buffer, then send the location to the buffer. 
	if (Parent != nullptr && bOn) {
		// Linear Acceleration & Velocity
		LinearVelocityThen = LinearVelocity;
		if(UseCOM){
			LinearVelocity = Parent->GetPhysicsLinearVelocity();
		}
		else{
			LinearVelocity  = Parent->GetPhysicsLinearVelocityAtPoint(this->GetComponentLocation());
		}
		LinearVelocity = ConvertLinearVector(LinearVelocity, UEToClient);
		LinearAcceleration = (LinearVelocity - LinearVelocityThen) / DeltaTime;

		// Position
		Position = this->GetComponentLocation();
		Position = ConvertLinearVector(Position, UEToClient);
		
		// Ang Acceleration & Velocity
		AngularVelocityThen = AngularVelocity;
		AngularVelocity = Parent->GetPhysicsAngularVelocityInRadians();
		AngularVelocity = ConvertAngularVector(AngularVelocity, NoScale);
		AngularAcceleration = (AngularVelocity - AngularVelocityThen) / DeltaTime;

		// Rotation
		if(UseCOM){
			Rotation = this->GetAttachParent()->GetComponentRotation().Quaternion();
		}
		else{
			Rotation = this->GetComponentRotation().Quaternion();
		}
		// TODO: Flip into right handed frame

		// Fill everything in
		float* FloatBuffer = static_cast<float*>(Buffer);
		FloatBuffer[0] = LinearAcceleration.X;
		FloatBuffer[1] = LinearAcceleration.Y;
		FloatBuffer[2] = LinearAcceleration.Z;
		FloatBuffer[3] = LinearVelocity.X;
		FloatBuffer[4] = LinearVelocity.Y;
		FloatBuffer[5] = LinearVelocity.Z;
		FloatBuffer[6] = Position.X;
		FloatBuffer[7] = Position.Y;
		FloatBuffer[8] = Position.Z;
		FloatBuffer[9] = AngularAcceleration.X;
		FloatBuffer[10] = AngularAcceleration.Y;
		FloatBuffer[11] = AngularAcceleration.Z;
		FloatBuffer[12] = AngularVelocity.X;
		FloatBuffer[13] = AngularVelocity.Y;
		FloatBuffer[14] = AngularVelocity.Z;
		FloatBuffer[15] = Rotation.W;
		FloatBuffer[16] = Rotation.X;
		FloatBuffer[17] = Rotation.Y;
		FloatBuffer[18] = Rotation.Z;
	}
}
