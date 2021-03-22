#include "Holodeck.h"
#include "SendAcousticMessageCommand.h"
#include "AcousticBeaconSensor.h"
#include "HolodeckGameMode.h"

void USendAcousticMessageCommand::Execute() {

	UE_LOG(LogHolodeck, Log, TEXT("SendAcousticMessageCommand::Execute"));

	verifyf(StringParams.size() == 2 && NumberParams.size() == 1, TEXT("USendAcousticMessageCommand::Execute: Invalid Arguments"));

	AHolodeckGameMode* GameTarget = static_cast<AHolodeckGameMode*>(Target);

	verifyf(GameTarget != nullptr, TEXT("%s UCommand::Target is not a UHolodeckGameMode*."), *FString(__func__));

	UWorld* World = Target->GetWorld();
	verify(World);

	FString AgentName = StringParams[0].c_str();
	float num = NumberParams[0];

	verifyf(num > 0, TEXT("%s Invalid ticks per capture provided!"), *FString(__func__));

	AHolodeckAgent* Agent = GetAgent(AgentName);

	verifyf(Agent, TEXT("%s Could not find agent %s"), *FString(__func__), *AgentName);

	FString SensorName = StringParams[1].c_str();

	verifyf(Agent->SensorMap.Contains(SensorName), TEXT("%s Sensor %s not found on agent %s"), *FString(__func__), *SensorName, *AgentName);

	UAcousticBeaconSensor* Camera = (UAcousticBeaconSensor*)Agent->SensorMap[SensorName];
	Camera->num = num;
}
