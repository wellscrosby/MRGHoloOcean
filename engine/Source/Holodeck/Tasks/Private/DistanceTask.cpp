#include "Holodeck.h"
#include "DistanceTask.h"
#include "Json.h"

// Set default values
void UDistanceTask::InitializeSensor() {
	Super::InitializeSensor();

	float Distance = CalcDistance();

	if (MaximizeDistance) {
		NextDistance = Distance + Interval;
	} else {
		NextDistance = Distance - Interval;
	}
}

// Allows sensor parameters to be set programmatically from client.
void UDistanceTask::ParseSensorParms(FString ParmsJson) {
	Super::ParseSensorParms(ParmsJson);

	TSharedPtr<FJsonObject> JsonParsed;
	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(ParmsJson);
	if (FJsonSerializer::Deserialize(JsonReader, JsonParsed)) {

		if (JsonParsed->HasTypedField<EJson::String>("DistanceActor")) {
			DistanceActorTag = JsonParsed->GetStringField("DistanceActor");
		}

		if (JsonParsed->HasTypedField<EJson::String>("GoalActor")) {
			GoalActorTag = JsonParsed->GetStringField("GoalActor");
		}

		if (JsonParsed->HasTypedField<EJson::Array>("GoalLocation")) {
			TArray<TSharedPtr<FJsonValue>> LocationArray = JsonParsed->GetArrayField("GoalLocation");
			if (LocationArray.Num() == 3) {
				double X, Y, Z;
				if (LocationArray[0]->TryGetNumber(X) && LocationArray[1]->TryGetNumber(Y) && LocationArray[2]->TryGetNumber(Z))
					GoalLocation = FVector(X, Y, Z);
			}
		}

		if (JsonParsed->HasTypedField<EJson::Number>("Interval")) {
			Interval = JsonParsed->GetNumberField("Interval");
		}

		if (JsonParsed->HasTypedField<EJson::Number>("GoalDistance")) {
			GoalDistance = JsonParsed->GetNumberField("GoalDistance");
		}

		if (JsonParsed->HasTypedField<EJson::Boolean>("MaximizeDistance")) {
			MaximizeDistance = JsonParsed->GetBoolField("MaximizeDistance");
		}
	} else {
		UE_LOG(LogHolodeck, Warning, TEXT("UDistanceTask::ParseSensorParms:: Unable to parse json."));
	}
}

// Called every frame
void UDistanceTask::TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	if (DistanceActor == nullptr && DistanceActorTag != "")
		DistanceActor = FindActorWithTag(DistanceActorTag);
	if (GoalActor == nullptr && GoalActorTag != "")
		GoalActor = FindActorWithTag(GoalActorTag);

	if ((DistanceActor || DistanceActorTag == "") && (GoalActor || GoalActorTag == "")) {
		float Distance = CalcDistance();

		if (MaximizeDistance) {
			if (Distance > NextDistance) {
				Reward = 1;
				NextDistance = Distance + Interval;
			}
			else {
				Reward = 0;
			}
			Terminal = Distance > GoalDistance;
		}
		else {
			if (Distance < NextDistance) {
				Reward = 1;
				NextDistance = Distance - Interval;
			}
			else {
				Reward = 0;
			}
			Terminal = Distance < GoalDistance;
		}
	}

	// Call TaskSensor's Tick to store Reward and Terminal in sensor buffer
	UTaskSensor::TickSensorComponent(DeltaTime, TickType, ThisTickFunction);
}

// Get Distance
float UDistanceTask::CalcDistance() {
	FVector FromLocation = this->GetComponentLocation();
	if (DistanceActor) {
		FromLocation = DistanceActor->GetActorLocation();
	}

	FVector ToLocation = GoalLocation;
	if (GoalActor) {
		ToLocation = GoalActor->GetActorLocation();
	}

	return (ToLocation - FromLocation).Size();
}
