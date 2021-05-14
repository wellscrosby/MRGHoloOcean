#pragma once
#include "Holodeck.h"
#include "HolodeckSensor.h"
#include "Kismet/KismetMathLibrary.h"

#include "OpticalModemSensor.generated.h"


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class HOLODECK_API UOpticalModemSensor : public UHolodeckSensor {
    GENERATED_BODY()

public:
    UOpticalModemSensor();

    virtual void InitializeSensor() override;
    UOpticalModemSensor* fromSensor = NULL;

	bool CanTransmit();


protected:
	int GetNumItems() override { return 1; };
    void TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere)
	int LaserMaxDistance = 1000;


private:

    UPrimitiveComponent* Parent;
    bool IsSensorOriented(FVector localToSensor);
};