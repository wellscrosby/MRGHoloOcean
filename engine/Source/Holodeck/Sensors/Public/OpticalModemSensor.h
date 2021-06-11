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
    UOpticalModemSensor* fromSensor = nullptr;

virtual void ParseSensorParms(FString ParmsJson) override;

protected:
	int GetNumItems() override { return 1; };

    int GetItemSize() override { return 1; }

    void TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    //Max distance of the modem in meters
    UPROPERTY(EditAnywhere)
	float MaxDistance = 50;
    
    //Debug Variables
    UPROPERTY(EditAnywhere)
	int LaserAngle = 60;

	UPROPERTY(EditAnywhere)
	bool LaserDebug = false;

    UPROPERTY(EditAnywhere)
    int DebugNumSides = 72;  //Default so that each side is 5 degrees

    UPROPERTY(EditAnywhere)
    FColor DebugColor = FColor::Green;


private:

    UPrimitiveComponent* Parent;
    bool IsSensorOriented(FVector localToSensor);
    int* CanTransmit();
    TMap<FString, FColor> ColorMap;
    void FillColorMap();
};