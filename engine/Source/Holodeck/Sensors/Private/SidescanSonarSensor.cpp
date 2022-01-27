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
	MaxRange = 10 * 100; 	// 10 m (in cm)
	MinRange = 0.1 * 100; 	// 0.1 m (in cm)
	Azimuth = 130; 			// degrees
	Elevation = 1;			// degrees

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

		if (JsonParsed->HasTypedField<EJson::Number>("BinsRange")) {
			BinsRange = JsonParsed->GetIntegerField("BinsRange");
		}

		if (JsonParsed->HasTypedField<EJson::Number>("BinsAzimuth")) {
			BinsAzimuth = JsonParsed->GetIntegerField("BinsAzimuth");
		}

		if (JsonParsed->HasTypedField<EJson::Number>("BinsElevation")) {
			BinsElevation = JsonParsed->GetIntegerField("BinsElevation");
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

	if(BinsElevation == 0){
		BinsElevation = Elevation*10;
	}
}

void USidescanSonarSensor::InitializeSensor() {
	Super::InitializeSensor();
	
	// Get size of each bin
	RangeRes = (MaxRange - MinRange) / BinsRange;
	AzimuthRes = Azimuth / BinsAzimuth;
	ElevRes = Elevation / BinsElevation;
	
	// setup count of each bin
	// count = new int32[BinsRange*BinsAzimuth](); // Method for imaging sonar
	count = new int32[BinsRange](); // Sidescan Sonar (1d array)

	for(int i=0;i<BinsAzimuth*BinsElevation;i++){
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
		// std::fill(result, result+BinsRange*BinsAzimuth, 0);
		// std::fill(count, count+BinsRange*BinsAzimuth, 0);
		std::fill(result, result+BinsRange, 0);
		std::fill(count, count+BinsRange, 0);
		
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
				l->idx.Y = (int32)((l->locSpherical.Y - minAzimuth)/ AzimuthRes);
				l->idx.Z = (int32)((l->locSpherical.Z - minElev)/ ElevRes);
				// Sometimes we get float->int rounding errors
				if(l->idx.Y == BinsAzimuth) --l->idx.Y;

				// UE_LOG(LogTemp, Warning, TEXT("Index Y: %d"), l->idx.Y);

				idx = l->idx.Z*BinsAzimuth + l->idx.Y;
				sortedLeaves[idx].Emplace(l);
			}
		}


		// HANDLE SHADOWING
		float eps = 16;
		ParallelFor(BinsAzimuth*BinsElevation, [&](int32 i){
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
				
				jth->idx.X = (int32)((jth->locSpherical.X - MinRange) / RangeRes);
				if (jth->idx.Y > (BinsAzimuth / 2))
				// if (jth->locSpherical.Y > 0)
				{
					idx = BinsRange / 2 - jth->idx.X / 2 - 1;
				}
				else
				{
					idx = BinsRange / 2 + jth->idx.X / 2;
				}
				// idx = jth->idx.X*BinsAzimuth + jth->idx.Y;
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
		// for (int i = 0; i < BinsRange*BinsAzimuth; i++) {
		for (int i = 0; i < BinsRange; i++) {
			if(count[i] != 0){
				result[i] *= (1 + multNoise.sampleFloat())/count[i];
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
			DrawDebugLine(GetWorld(), spherToEucSS(MinRange, minAzimuth, minElev, tran), spherToEucSS(MaxRange, minAzimuth, minElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, debugThickness);
			DrawDebugLine(GetWorld(), spherToEucSS(MinRange, minAzimuth, maxElev, tran), spherToEucSS(MaxRange, minAzimuth, maxElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, debugThickness);
			DrawDebugLine(GetWorld(), spherToEucSS(MinRange, maxAzimuth, minElev, tran), spherToEucSS(MaxRange, maxAzimuth, minElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, debugThickness);
			DrawDebugLine(GetWorld(), spherToEucSS(MinRange, maxAzimuth, maxElev, tran), spherToEucSS(MaxRange, maxAzimuth, maxElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, debugThickness);

			// azimuth lines (should be arcs, we're being lazy)
			DrawDebugLine(GetWorld(), spherToEucSS(MinRange, minAzimuth, minElev, tran), spherToEucSS(MinRange, maxAzimuth, minElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, debugThickness);
			DrawDebugLine(GetWorld(), spherToEucSS(MinRange, minAzimuth, maxElev, tran), spherToEucSS(MinRange, maxAzimuth, maxElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, debugThickness);
			DrawDebugLine(GetWorld(), spherToEucSS(MaxRange, minAzimuth, minElev, tran), spherToEucSS(MaxRange, maxAzimuth, minElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, debugThickness);
			DrawDebugLine(GetWorld(), spherToEucSS(MaxRange, minAzimuth, maxElev, tran), spherToEucSS(MaxRange, maxAzimuth, maxElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, debugThickness);

			// elevation lines (should be arcs, we're being lazy)
			DrawDebugLine(GetWorld(), spherToEucSS(MinRange, minAzimuth, minElev, tran), spherToEucSS(MinRange, minAzimuth, maxElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, debugThickness);
			DrawDebugLine(GetWorld(), spherToEucSS(MinRange, maxAzimuth, minElev, tran), spherToEucSS(MinRange, maxAzimuth, maxElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, debugThickness);
			DrawDebugLine(GetWorld(), spherToEucSS(MaxRange, minAzimuth, minElev, tran), spherToEucSS(MaxRange, minAzimuth, maxElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, debugThickness);
			DrawDebugLine(GetWorld(), spherToEucSS(MaxRange, maxAzimuth, minElev, tran), spherToEucSS(MaxRange, maxAzimuth, maxElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, debugThickness);
		}		
	}
}
