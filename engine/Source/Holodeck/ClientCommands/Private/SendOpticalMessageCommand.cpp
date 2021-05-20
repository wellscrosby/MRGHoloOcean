#include "Holodeck.h"
#include "SendOpticalMessageCommand.h"
#include "OpticalModemSensor.h"
#include "HolodeckGameMode.h"

void USendOpticalMessageCommand::Execute() {

	UE_LOG(LogHolodeck, Log, TEXT("SendOpticalMessageCommand::Execute"));

	// Verify things and get everything set up 
	verifyf(StringParams.size() == 4 && NumberParams.size() == 0, TEXT("USendOpticalMessageCommand::Execute: Invalid Arguments"));
	AHolodeckGameMode* GameTarget = static_cast<AHolodeckGameMode*>(Target);
	verifyf(GameTarget != nullptr, TEXT("%s UCommand::Target is not a UHolodeckGameMode*."), *FString(__func__));
	UWorld* World = Target->GetWorld();
	verify(World);

	// Get sensor it came from
	FString fromAgentName = StringParams[0].c_str();
	FString fromSensorName = StringParams[1].c_str();

	AHolodeckAgent* fromAgent = GetAgent(fromAgentName);
	verifyf(fromAgent, TEXT("%s Could not find agent %s"), *FString(__func__), *fromAgentName);
	verifyf(fromAgent->SensorMap.Contains(fromSensorName), TEXT("%s Sensor %s not found on agent %s"), *FString(__func__), *fromSensorName, *fromAgentName);
	UOpticalModemSensor* fromSensor = (UOpticalModemSensor*)fromAgent->SensorMap[fromSensorName];

	// Get sensor where it's going
	FString toAgentName = StringParams[2].c_str();
	FString toSensorName = StringParams[3].c_str();

	AHolodeckAgent* toAgent = GetAgent(toAgentName);
	verifyf(toAgent, TEXT("%s Could not find agent %s"), *FString(__func__), *toAgentName);
	verifyf(toAgent->SensorMap.Contains(toSensorName), TEXT("%s Sensor %s not found on agent %s"), *FString(__func__), *toSensorName, *toAgentName);
	UOpticalModemSensor* toSensor = (UOpticalModemSensor*)toAgent->SensorMap[toSensorName];

	// Send the message
	toSensor->fromSensor = fromSensor;
}