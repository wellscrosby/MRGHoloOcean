// MIT License (c) 2019 BYU PCCL see LICENSE file

#include "Holodeck.h"
#include "PoseSensor.h"

UPoseSensor::UPoseSensor() {
	PrimaryComponentTick.bCanEverTick = true;
	SensorName = "PoseSensor";
}

void UPoseSensor::InitializeSensor() {
	Super::InitializeSensor();

	Controller = static_cast<AHolodeckPawnController*>(this->GetAttachmentRootActor()->GetInstigator()->Controller);
	Parent = static_cast<UPrimitiveComponent*>(this->GetAttachParent());
	RootMesh = static_cast<UStaticMeshComponent*>(this->GetAttachParent());

	World = Parent->GetWorld();
}

// Called every frame
void UPoseSensor::TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	if (Parent != nullptr && RootMesh != nullptr) {
		FVector Forward = RootMesh->GetForwardVector();
		FVector Right = RootMesh->GetRightVector();
		FVector Up = RootMesh->GetUpVector();
		FVector Location = RootMesh->GetComponentLocation();

		float* FloatBuffer = static_cast<float*>(Buffer);
		Forward = ConvertLinearVector(Forward, NoScale);
		FVector Left = ConvertLinearVector(-Right, NoScale);
		Up = ConvertLinearVector(Up, NoScale);
		Location = ConvertLinearVector(Location, UEToClient);

		// Insert Rotation Matrix
		FloatBuffer[0] = Forward.X;
		FloatBuffer[4] = Forward.Y;
		FloatBuffer[8] = Forward.Z;
		FloatBuffer[1] = Left.X;
		FloatBuffer[5] = Left.Y;
		FloatBuffer[9] = Left.Z;
		FloatBuffer[2] = Up.X;
		FloatBuffer[6] = Up.Y;
		FloatBuffer[10] = Up.Z;

		// Insert Position
		FloatBuffer[3] = Location.X;
		FloatBuffer[7] = Location.Y;
		FloatBuffer[11] = Location.Z;

		// Insert Bottom Row
		FloatBuffer[12] = 0;
		FloatBuffer[13] = 0;
		FloatBuffer[14] = 0;
		FloatBuffer[15] = 1;
	}
}
