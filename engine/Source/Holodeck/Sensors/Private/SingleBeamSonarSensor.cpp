// MIT License (c) 2021 BYU FRoStLab see LICENSE file

#include "Holodeck.h"
#include "Benchmarker.h"
#include "HolodeckBuoyantAgent.h"
#include "SingleBeamSonarSensor.h"
// #pragma warning (disable : 4101)

USingleBeamSonarSensor::USingleBeamSonarSensor() {
	SensorName = "SingleBeamSonarSensor";
} 

void USingleBeamSonarSensor::BeginDestroy() {
	Super::BeginDestroy();

	delete[] count;
}

// Allows sensor parameters to be set programmatically from client.
void USingleBeamSonarSensor::ParseSensorParms(FString ParmsJson) {

	// default values
	OpeningAngle = 30;
	
	RangeBins = 200;
	BinsCentralAngle = 6;
	BinsOpeningAngle = 5;

	// range in cm
	RangeMin = 50;
	RangeMax = 1000;

	Super::ParseSensorParms(ParmsJson);

	// user can override default values
	TSharedPtr<FJsonObject> JsonParsed;
	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(ParmsJson);
	if (FJsonSerializer::Deserialize(JsonReader, JsonParsed)) {
		// Geometry Parameters
		if (JsonParsed->HasTypedField<EJson::Number>("RangeBins")) {
			RangeBins = JsonParsed->GetIntegerField("RangeBins");
		} 	
		if (JsonParsed->HasTypedField<EJson::Number>("BinsOpeningAngle")) {
			BinsOpeningAngle = JsonParsed->GetIntegerField("BinsOpeningAngle");
		}
		if (JsonParsed->HasTypedField<EJson::Number>("BinsCentralAngle")) {
			BinsCentralAngle = JsonParsed->GetIntegerField("BinsCentralAngle");
		}
		if (JsonParsed->HasTypedField<EJson::Number>("OpeningAngle")) {
			OpeningAngle = JsonParsed->GetIntegerField("OpeningAngle");
		}


		// Noise Parameters
		if (JsonParsed->HasTypedField<EJson::Number>("AddSigma")) {
			addNoise.initSigma(JsonParsed->GetNumberField("AddSigma"));
		}
		if (JsonParsed->HasTypedField<EJson::Number>("AddCov")) {
			addNoise.initCov(JsonParsed->GetNumberField("AddCov"));
		}
		if (JsonParsed->HasTypedField<EJson::Number>("MultSigma")) {
			multNoise.initSigma(JsonParsed->GetNumberField("MultSigma"));
		}
		if (JsonParsed->HasTypedField<EJson::Number>("MultCov")) {
			multNoise.initCov(JsonParsed->GetNumberField("MultCov"));
		}
		if (JsonParsed->HasTypedField<EJson::Number>("RangeSigma")) {
			rNoise.initBounds(JsonParsed->GetNumberField("RangeSigma"));
		}
		if (JsonParsed->HasTypedField<EJson::Number>("RangeCov")) {
			rNoise.initBounds(JsonParsed->GetNumberField("RangeCov"));
		}
	}
	else {
		UE_LOG(LogHolodeck, Fatal, TEXT("USingleBeamSonarSensor::ParseSensorParms:: Unable to parse json."));
	}

	if(BinsOpeningAngle == 0){
		BinsOpeningAngle = OpeningAngle*10;
	}
}

void USingleBeamSonarSensor::InitializeSensor() {
	Super::InitializeSensor();
	
	// Setup bins
	// TODO: Make these according to octree size
	CentralAngle = 360;

	minCentralAngle = -180;
	maxCentralAngle = 180;
	minOpeningAngle = 0;
	maxOpeningAngle = OpeningAngle/2;

	// Get size of each bin
	RangeRes = (RangeMax - RangeMin) / RangeBins;
	CentralAngleRes = CentralAngle / BinsCentralAngle;
	OpeningAngleRes = OpeningAngle / BinsOpeningAngle;
	
	// setup count of each bin
	count = new int32[RangeBins]();

	for(int i=0;i<BinsCentralAngle*BinsOpeningAngle;i++){
		sortedLeaves.Add(TArray<Octree*>());
		sortedLeaves[i].Reserve(10000);
	}

	sqrt3_2 = UKismetMathLibrary::Sqrt(3)/2;
	sinOffset = UKismetMathLibrary::DegSin(FGenericPlatformMath::Min(CentralAngle, OpeningAngle)/2);
}


// determine if a single leaf is in your tree
bool USingleBeamSonarSensor::inRange(Octree* tree){
	FTransform SensortoWorld = this->GetComponentTransform();
	// if it's not a leaf, we use a bigger search area
	float offset = 0;
	float radius = 0;

	if(tree->size != Octree::OctreeMin){
		radius = tree->size*sqrt3_2;
		offset = radius/sinOffset;
		SensortoWorld.AddToTranslation( -this->GetForwardVector()*offset );
	}
	
	// transform location to sensor frame instead of global (x y z)
	FVector locLocal = SensortoWorld.GetRotation().UnrotateVector(tree->loc-SensortoWorld.GetTranslation());

	// check if it's in range
	tree->locSpherical.X = locLocal.Size();
	if(RangeMin+offset-radius >= tree->locSpherical.X || tree->locSpherical.X >= RangeMax+offset+radius) return false; 

	// check if OpeningAngle is in range. OpeningAngle is angle off of x-axis
	tree->locSpherical.Z = ATan2Approx(UKismetMathLibrary::Sqrt(UKismetMathLibrary::Square(locLocal.Y)+UKismetMathLibrary::Square(locLocal.Z)), locLocal.X); //OpeningAngle of leaf we are inspecting
	if(minOpeningAngle >= tree->locSpherical.Z || tree->locSpherical.Z >= maxOpeningAngle) return false;

	// save CentralAngle for shadowing later. CentralAngle goes around the x-axis
	tree->locSpherical.Y = ATan2Approx(locLocal.Z, locLocal.Y);

	// otherwise it's in!
	return true;
}	


void USingleBeamSonarSensor::showRegion(float DeltaTime){
	if(ViewRegion){
		float debugThickness = 3.0f;
		float DebugNumSides = 6; //change later?
		float length = (RangeMax - RangeMin); //length of cone in cm

		DrawDebugCone(GetWorld(), GetComponentLocation(), GetForwardVector(), length, (OpeningAngle/2)*Pi/180, (OpeningAngle/2)*Pi/180, DebugNumSides, FColor::Green, false, .00, ECC_WorldStatic, debugThickness);
	}		
}

void USingleBeamSonarSensor::TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	Super::TickSensorComponent(DeltaTime, TickType, ThisTickFunction);

	if(TickCounter == 0){
		// reset things and get ready
		float* result = static_cast<float*>(Buffer);
		std::fill(result, result+RangeBins, 0);
		std::fill(count, count+RangeBins, 0);
		
		for(auto& sl: sortedLeaves){
			sl.Reset();
		}

		// Finds leaves in range and puts them in foundLeaves
		findLeaves();		// does not return anything, saves to foundLeaves


		// SORT THEM INTO CENTRALANGLE/OPENINGANGLE BINS
		int32 idx;
		for(TArray<Octree*>& bin : foundLeaves){
			for(Octree* l : bin){
				// Compute bins while we're parallelized
				l->idx.Y = (int32)((l->locSpherical.Y - minCentralAngle)/ CentralAngleRes);
				l->idx.Z = (int32)((l->locSpherical.Z - minOpeningAngle)/ OpeningAngleRes);
				// Sometimes we get float->int rounding errors
				if(l->idx.Y == BinsCentralAngle) --l->idx.Y;

				idx = l->idx.Z*BinsCentralAngle + l->idx.Y;
				// array of arrays (the rectangle we split off)
				sortedLeaves[idx].Emplace(l);
			}
		}

		// HANDLE SHADOWING
		shadowLeaves();


		// ADD IN ALL CONTRIBUTIONS
		float range_noise;
		for(TArray<Octree*>& bin : sortedLeaves){
			for(Octree* l : bin){
				// Add noise to each of them
				range_noise = rNoise.sampleExponential();
				l->idx.X = (int32)((l->locSpherical.X - RangeMin + range_noise) / RangeRes); 

				// In case our noise has pushed us out of range
				if(l->idx.X >= RangeBins) l->idx.X = RangeBins-1;

				// Add to their appropriate bin
				idx = l->idx.X;

				result[idx] += l->val;
				++count[idx];
			}
		}
		

		// MOVE THEM INTO BUFFER
		for (int i = 0; i < RangeBins; i++) {
			if(count[i] != 0){

				// actually take the average of the intensities
				result[i] *= (1 + multNoise.sampleFloat())/count[i];
				result[i] += addNoise.sampleRayleigh();
			}
			else{
				result[i] = addNoise.sampleRayleigh();
			}
		}		
	}
}
