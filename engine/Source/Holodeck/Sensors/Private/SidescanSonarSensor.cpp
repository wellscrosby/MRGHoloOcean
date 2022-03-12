// MIT License (c) 2021 BYU FRoStLab see LICENSE file

#include "Holodeck.h"
#include "Benchmarker.h"
#include "HolodeckBuoyantAgent.h"
#include "SidescanSonarSensor.h"
// #pragma warning (disable : 4101)

USidescanSonarSensor::USidescanSonarSensor() {
	SensorName = "SidescanSonarSensor";
}

void USidescanSonarSensor::BeginDestroy() {
	Super::BeginDestroy();

	delete[] count;
}

// Allows sensor parameters to be set programmatically from client.
void USidescanSonarSensor::ParseSensorParms(FString ParmsJson) {

	// Override the Parent Class defaults for some key parameters
	Azimuth = 170; 			// degrees
	Elevation = 0.25;		// degrees
	RangeMin = 0.5 * 100; 	// 0.5 m (in cm)
	RangeMax = 35 * 100; 	// 35 m (in cm)

	// Parse any parent class parameters that were provided in the configuration
	Super::ParseSensorParms(ParmsJson);

	TSharedPtr<FJsonObject> JsonParsed;
	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(ParmsJson);
	if (FJsonSerializer::Deserialize(JsonReader, JsonParsed)) {

		// For handling noise
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

		if (JsonParsed->HasTypedField<EJson::Number>("RangeRes")) {
			RangeRes = JsonParsed->GetNumberField("RangeRes") * 100; // m to cm
		}

		if (JsonParsed->HasTypedField<EJson::Number>("ElevationRes")) {
			ElevationRes = JsonParsed->GetNumberField("ElevationRes"); // degrees
		}

		if (JsonParsed->HasTypedField<EJson::Number>("AzimuthRes")) {
			AzimuthRes = JsonParsed->GetNumberField("AzimuthRes"); // degrees
		}

		if (JsonParsed->HasTypedField<EJson::Number>("RangeBins")) {
			RangeBins = JsonParsed->GetIntegerField("RangeBins");
		}

		if (JsonParsed->HasTypedField<EJson::Number>("AzimuthBins")) {
			AzimuthBins = JsonParsed->GetIntegerField("AzimuthBins");
		}

		if (JsonParsed->HasTypedField<EJson::Number>("ElevationBins")) {
			ElevationBins = JsonParsed->GetIntegerField("ElevationBins");
		}

		if (JsonParsed->HasTypedField<EJson::Boolean>("ViewRegion")) {
			ViewRegion = JsonParsed->GetBoolField("ViewRegion");
		}

		if (JsonParsed->HasTypedField<EJson::Number>("ViewOctree")) {
			ViewOctree = JsonParsed->GetIntegerField("ViewOctree");
		}

	}
	else {
		UE_LOG(LogHolodeck, Fatal, TEXT("USidescanSonarSensor::ParseSensorParms:: Unable to parse json."));
	}

	// Parse through the Range parameters given to us
	if(RangeBins != 0){
		RangeRes = (RangeMax - RangeMin) / RangeBins;
	} 
	else if(RangeRes != 0){
		RangeBins = (RangeMax - RangeMin) / RangeRes;
	}
	else{
		RangeRes = 5; // cm
		RangeBins = (RangeMax - RangeMin) / RangeRes;
	}

	// Parse through the Azimuth parameters given to us
	if(AzimuthBins != 0){
		AzimuthRes = Azimuth / AzimuthBins;
	} 
	else if(AzimuthRes != 0){
		AzimuthBins = Azimuth / AzimuthRes;
	}
	else{
		// Calculate how large the azimuth bins should be
		AzimuthRes = (180 * 1.5 * Octree::OctreeMin) / (Pi * RangeMin);
		AzimuthBins = Azimuth / AzimuthRes;
	}

	// Parse through the Elevation parameters given to us
	if(ElevationBins != 0){
		ElevationRes = Elevation / ElevationBins;
	} 
	else if(ElevationRes != 0){
		ElevationBins = Elevation / ElevationRes;
	}
	else{
		// Calculate how large our shadowing bins should be
		ElevationBins = (RangeMin*Elevation*Pi/180) / Octree::OctreeMin;
		if(ElevationBins < 1) ElevationBins = 1;
		ElevationRes = Elevation / ElevationBins;
	}
}

void USidescanSonarSensor::InitializeSensor() {
	Super::InitializeSensor();
	
	// setup count of each bin
	// count = new int32[RangeBins*AzimuthBins](); // Method for imaging sonar
	count = new int32[RangeBins](); // Sidescan Sonar (1d array)

	for(int i=0;i<AzimuthBins*ElevationBins;i++){
		sortedLeaves.Add(TArray<Octree*>());
		sortedLeaves[i].Reserve(10000);
	}
}

// Conversion from Spherical coordinates to Euclidian
FVector spherToEucSS(float r, float theta, float phi, FTransform SensortoWorld){
	float x = r*UKismetMathLibrary::DegSin(phi)*UKismetMathLibrary::DegCos(theta);
	float y = r*UKismetMathLibrary::DegSin(phi)*UKismetMathLibrary::DegSin(theta);
	float z = r*UKismetMathLibrary::DegCos(phi);
	return UKismetMathLibrary::TransformLocation(SensortoWorld, FVector(x, y, z));
}


void USidescanSonarSensor::TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	Super::TickSensorComponent(DeltaTime, TickType, ThisTickFunction);

	if(TickCounter == 0){
		// reset things and get ready
		float* result = static_cast<float*>(Buffer);
		// std::fill(result, result+RangeBins*AzimuthBins, 0);
		// std::fill(count, count+RangeBins*AzimuthBins, 0);
		std::fill(result, result+RangeBins, 0);
		std::fill(count, count+RangeBins, 0);
		
		for(auto& sl: sortedLeaves){
			sl.Reset();
		}

		// Finds leaves in range and puts them in foundLeaves
		findLeaves();		


		// SORT THEM INTO AZIMUTH/ELEVATION BINS
		int32 idx;
		for(TArray<Octree*>& bin : foundLeaves){
			for(Octree* l : bin){
				// Compute bins while we're parallelized
				l->idx.Y = (int32)((l->locSpherical.Y - minAzimuth)/ AzimuthResDegrees);
				l->idx.Z = (int32)((l->locSpherical.Z - minElev)/ ElevationResDegrees);
				// Sometimes we get float->int rounding errors
				if(l->idx.Y == AzimuthBins) --l->idx.Y;

				// UE_LOG(LogTemp, Warning, TEXT("Index Y: %d"), l->idx.Y);

				idx = l->idx.Z*AzimuthBins + l->idx.Y;
				sortedLeaves[idx].Emplace(l);
			}
		}


		// HANDLE SHADOWING
		float eps = 16;
		ParallelFor(AzimuthBins*ElevationBins, [&](int32 i){
			TArray<Octree*>& binLeafs = sortedLeaves.GetData()[i]; 

			// sort from closest to farthest
			binLeafs.Sort([](const Octree& a, const Octree& b){
				return a.locSpherical.X < b.locSpherical.X;
			});

			// Get the closest cluster in the bin
			int32 idx;
			float diff, z_t, R;
			float z_i = sos_water * density_water;
			Octree* jth;
			for(int j=0;j<binLeafs.Num()-1;j++){
				jth = binLeafs.GetData()[j];
				
				jth->idx.X = (int32)((jth->locSpherical.X - RangeMin) / RangeRes);
				if (jth->idx.Y > (AzimuthBins / 2))
				// if (jth->locSpherical.Y > 0)
				{
					idx = RangeBins / 2 - jth->idx.X / 2 - 1;
				}
				else
				{
					idx = RangeBins / 2 + jth->idx.X / 2;
				}
				// idx = jth->idx.X*AzimuthBins + jth->idx.Y;
				z_t = jth->sos * jth->density;
				R = (z_t - z_i) / (z_t + z_i);

				result[idx] += jth->val * R;
				++count[idx];
				
				// diff = FVector::Dist(jth->loc, binLeafs.GetData()[j+1]->loc);
				diff = FMath::Abs(jth->locSpherical.X - binLeafs.GetData()[j+1]->locSpherical.X);
				if(diff > eps) break;
			}

		});

		
		// MOVE THEM INTO BUFFER
		// for (int i = 0; i < RangeBins*AzimuthBins; i++) {
		for (int i = 0; i < RangeBins; i++) {
			if(count[i] != 0){
				result[i] *= (1 + multNoise.sampleFloat()) / count[i];
				result[i] += addNoise.sampleRayleigh();
			}
			else{
				result[i] = addNoise.sampleRayleigh();
			}
		}


		// draw points inside our region
		if(ViewOctree >= -1){
			for( TArray<Octree*> bins : sortedLeaves){
				for( Octree* l : bins){
					if(ViewOctree == -1 || ViewOctree == l->idx.Y)
						DrawDebugPoint(GetWorld(), l->loc, 5, FColor::Red, false, DeltaTime*TicksPerCapture);
				}
			}
		}

		// draw outlines of our region
		if(ViewRegion){
			FTransform tran = this->GetComponentTransform();
			float debugThickness = 3.0f;
			
			// range lines
			DrawDebugLine(GetWorld(), spherToEucSS(RangeMin, minAzimuth, minElev, tran), spherToEucSS(RangeMax, minAzimuth, minElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, debugThickness);
			DrawDebugLine(GetWorld(), spherToEucSS(RangeMin, minAzimuth, maxElev, tran), spherToEucSS(RangeMax, minAzimuth, maxElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, debugThickness);
			DrawDebugLine(GetWorld(), spherToEucSS(RangeMin, maxAzimuth, minElev, tran), spherToEucSS(RangeMax, maxAzimuth, minElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, debugThickness);
			DrawDebugLine(GetWorld(), spherToEucSS(RangeMin, maxAzimuth, maxElev, tran), spherToEucSS(RangeMax, maxAzimuth, maxElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, debugThickness);

			// azimuth lines (should be arcs, we're being lazy)
			DrawDebugLine(GetWorld(), spherToEucSS(RangeMin, minAzimuth, minElev, tran), spherToEucSS(RangeMin, maxAzimuth, minElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, debugThickness);
			DrawDebugLine(GetWorld(), spherToEucSS(RangeMin, minAzimuth, maxElev, tran), spherToEucSS(RangeMin, maxAzimuth, maxElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, debugThickness);
			DrawDebugLine(GetWorld(), spherToEucSS(RangeMax, minAzimuth, minElev, tran), spherToEucSS(RangeMax, maxAzimuth, minElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, debugThickness);
			DrawDebugLine(GetWorld(), spherToEucSS(RangeMax, minAzimuth, maxElev, tran), spherToEucSS(RangeMax, maxAzimuth, maxElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, debugThickness);

			// elevation lines (should be arcs, we're being lazy)
			DrawDebugLine(GetWorld(), spherToEucSS(RangeMin, minAzimuth, minElev, tran), spherToEucSS(RangeMin, minAzimuth, maxElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, debugThickness);
			DrawDebugLine(GetWorld(), spherToEucSS(RangeMin, maxAzimuth, minElev, tran), spherToEucSS(RangeMin, maxAzimuth, maxElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, debugThickness);
			DrawDebugLine(GetWorld(), spherToEucSS(RangeMax, minAzimuth, minElev, tran), spherToEucSS(RangeMax, minAzimuth, maxElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, debugThickness);
			DrawDebugLine(GetWorld(), spherToEucSS(RangeMax, maxAzimuth, minElev, tran), spherToEucSS(RangeMax, maxAzimuth, maxElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, debugThickness);
		}		
	}
}
