// MIT License (c) 2021 BYU FRoStLab see LICENSE file

#include "Holodeck.h"
#include "Benchmarker.h"
#include "HolodeckBuoyantAgent.h"
#include "ImagingSonarSensor.h"
// #pragma warning (disable : 4101)

UImagingSonarSensor::UImagingSonarSensor() {
	SensorName = "ImagingSonarSensor";
}

void UImagingSonarSensor::BeginDestroy() {
	Super::BeginDestroy();

	delete[] count;
}

// Allows sensor parameters to be set programmatically from client.
void UImagingSonarSensor::ParseSensorParms(FString ParmsJson) {
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
		UE_LOG(LogHolodeck, Fatal, TEXT("UImagingSonarSensor::ParseSensorParms:: Unable to parse json."));
	}

	if(BinsElevation == 0){
		BinsElevation = Elevation*10;
	}
}

void UImagingSonarSensor::InitializeSensor() {
	Super::InitializeSensor();

	// Setup densities
	z_water = density_water * sos_water;
	
	// Get size of each bin
	RangeRes = (MaxRange - MinRange) / BinsRange;
	AzimuthRes = Azimuth / BinsAzimuth;
	ElevRes = Elevation / BinsElevation;
	
	// setup count of each bin
	count = new int32[BinsRange*BinsAzimuth]();

	for(int i=0;i<BinsAzimuth*BinsElevation;i++){
		sortedLeaves.Add(TArray<Octree*>());
		sortedLeaves[i].Reserve(10000);
	}

	mapLeaves.Reserve(100000);
}

FVector spherToEuc(float r, float theta, float phi, FTransform SensortoWorld){
	float x = r*UKismetMathLibrary::DegSin(phi)*UKismetMathLibrary::DegCos(theta);
	float y = r*UKismetMathLibrary::DegSin(phi)*UKismetMathLibrary::DegSin(theta);
	float z = r*UKismetMathLibrary::DegCos(phi);
	return UKismetMathLibrary::TransformLocation(SensortoWorld, FVector(x, y, z));
}


void UImagingSonarSensor::TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	Super::TickSensorComponent(DeltaTime, TickType, ThisTickFunction);

	if(TickCounter == 0){
		// reset things and get ready
		float* result = static_cast<float*>(Buffer);
		std::fill(result, result+BinsRange*BinsAzimuth, 0);
		std::fill(count, count+BinsRange*BinsAzimuth, 0);
		
		for(auto& sl: sortedLeaves){
			sl.Reset();
		}
		mapLeaves.Reset();
		mapSearch.Reset();
		cluster.Reset();


		// Finds leaves in range and puts them in foundLeaves
		Benchmarker b, c;
		findLeaves();		
		UE_LOG(LogHolodeck, Warning, TEXT("FIND LEAVES : %f"), c.CalcMs());

		// SORT THEM INTO AZIMUTH/ELEVATION BINS
		int32 idx;
		for(TArray<Octree*>& bin : foundLeaves){
			for(Octree* l : bin){
				// Compute bins while we're parallelized
				l->idx.Y = (int32)((l->locSpherical.Y - minAzimuth)/ AzimuthRes);
				l->idx.Z = (int32)((l->locSpherical.Z - minElev)/ ElevRes);
				// Sometimes we get float->int rounding errors
				if(l->idx.Y == BinsAzimuth) --l->idx.Y;

				idx = l->idx.Z*BinsAzimuth + l->idx.Y;
				sortedLeaves[idx].Emplace(l);
			}
		}
		UE_LOG(LogHolodeck, Warning, TEXT("BINS SORTING : %f"), c.CalcMs());

		// HANDLE SHADOWING
		float eps = RangeRes;
		ParallelFor(BinsAzimuth*BinsElevation, [&](int32 i){
			TArray<Octree*>& binLeafs = sortedLeaves.GetData()[i]; 

			// sort from closest to farthest
			binLeafs.Sort([](const Octree& a, const Octree& b){
				return a.locSpherical.X < b.locSpherical.X;
			});

			// Get the closest cluster in the bin
			float diff;
			Octree* jth;
			FIntVector last_idx = FIntVector(0,0,0);
			for(int j=0;j<binLeafs.Num();j++){
				jth = binLeafs.GetData()[j];
				
				jth->idx.X = (int32)((jth->locSpherical.X - MinRange) / RangeRes);
				jth->val *= (jth->z - z_water) / (jth->z + z_water);

				// diff = FVector::Dist(jth->loc, binLeafs.GetData()[j+1]->loc);
				if(j != binLeafs.Num()-1){
					diff = FMath::Abs(jth->locSpherical.X - binLeafs.GetData()[j+1]->locSpherical.X);
					if(diff > eps){
						binLeafs.RemoveAt(j+1,binLeafs.Num()-j-1);
						break;
					}
				}
			}

		});

		UE_LOG(LogHolodeck, Warning, TEXT("SHADOWING : %f"), c.CalcMs());

		// ADD IN ALL CONTRIBUTIONS
		int extraContr = 1;
		for(TArray<Octree*>& bin : sortedLeaves){
			for(Octree* l : bin){
				for(int i=FGenericPlatformMath::Max(0,l->idx.X-extraContr); i<FGenericPlatformMath::Min(BinsRange,l->idx.X+extraContr+1); i++){
					for(int j=FGenericPlatformMath::Max(0,l->idx.Y-extraContr); j<FGenericPlatformMath::Min(BinsAzimuth,l->idx.Y+extraContr+1); j++){
						idx = i*BinsAzimuth + j;
						result[idx] += l->val;
						++count[idx];
					}
				}
			}
		}

		UE_LOG(LogHolodeck, Warning, TEXT("ADDING IN REGULAR : %f"), c.CalcMs());

		// PUT INTO MAP FOR CLUSTER
		// TODO: Make a copy to search through, or add to 2 maps now?
		for(TArray<Octree*>& binLeafs : sortedLeaves){
			if(binLeafs.Num() > 0){
				Octree* jth = binLeafs.GetData()[0];
				mapLeaves.Add(jth->idx, jth);
				int idxR = jth->idx.X;
				for(int i=1;i<binLeafs.Num();i++){
					jth = binLeafs.GetData()[i];
					if(jth->idx.X != idxR){
						mapLeaves.Add(jth->idx, jth);
						idxR = jth->idx.X;
					}
				}
			}
		}
		UE_LOG(LogHolodeck, Warning, TEXT("MAKE MAP : %f"), c.CalcMs());

		// PUT THEM INTO CLUSTERS
		mapSearch = TMap<FIntVector,Octree*>(mapLeaves);
		int extra = 10;
		while(mapSearch.Num() > 0){
			// Get start of cluster
			Octree* l = mapSearch.begin()->Value;
			mapSearch.Remove(l->idx);
			cluster.Add({l});

			// Get anything that may be nearby
			for(int i=FGenericPlatformMath::Max(0,l->idx.X-extra); i<FGenericPlatformMath::Min(BinsRange,l->idx.X+extra+1); i++){
				for(int j=FGenericPlatformMath::Max(0,l->idx.Y-extra); j<FGenericPlatformMath::Min(BinsAzimuth,l->idx.Y+extra+1); j++){
					for(int k=FGenericPlatformMath::Max(0,l->idx.Z-extra); k<FGenericPlatformMath::Min(BinsElevation,l->idx.Z+extra+1); k++){
						Octree** close = mapSearch.Find(FIntVector(i,j,k));
						if(close != nullptr && FVector::DotProduct(l->normal, (*close)->normal) > 0.965){
							cluster.Top().Add(*close);
							mapSearch.Remove((*close)->idx);
						}
					}
				}
			}
		}
		UE_LOG(LogHolodeck, Warning, TEXT("CLUSTER THEM : %f"), c.CalcMs());

		// MULTIPATH CONTRIBUTIONS
		int step_size = RangeRes;
		int iterations = 400 / step_size;
		FTransform SensortoWorld = this->GetComponentTransform();
		std::function<FVector(FVector,FVector)> reflect;
		reflect = [](FVector normal, FVector impact){
			return -impact + 2*FVector::DotProduct(normal,impact)*normal;
		};
		ParallelFor(cluster.Num(), [&](int32 i){
			TArray<Octree*>& thisCluster = cluster.GetData()[i];
			Octree* l = thisCluster.GetData()[0];

			FVector reflection = reflect(l->normal, l->normalImpact);
			Octree stepper(l->loc, l->size);
			Octree** hit = nullptr; 
			FVector offset = reflection*step_size*30;

			// TODO: Replace this with real raytracing?
			for(int j=0;j<iterations;j++){
				// step
				offset += reflection*step_size;
				stepper.loc = l->loc + offset;

				// make sure it's still in range (& compute spherical coordinates)
				if(!inRange(&stepper)){
					thisCluster.Empty();
					return;
				}

				// Set the index values
				stepper.idx.X = (int32)((stepper.locSpherical.X - MinRange) / RangeRes);
				stepper.idx.Y = (int32)((stepper.locSpherical.Y - minAzimuth)/ AzimuthRes);
				stepper.idx.Z = (int32)((stepper.locSpherical.Z - minElev)/ ElevRes);

				// If there's something in that bin
				hit = mapLeaves.Find(stepper.idx);
				if(hit != nullptr){
					FVector returnImpact = reflect((*hit)->normal, -reflection);
					// make sure it's in the right direction
					if(FVector::DotProduct(returnImpact, (*hit)->normalImpact) > 0) break;
					else {
						thisCluster.Empty();
						return;
					}
				}
			} 

			if(hit == nullptr){
				thisCluster.Empty();
				return;
			}

			// If we did hit something, ray trace the rest of everything in the cluster
			float t;
			FVector locBounce, returnRay;
			for(Octree* m : thisCluster){
				// find 2nd impact location
				reflection = reflect(m->normal, m->normalImpact);
				t = FVector::DotProduct((*hit)->loc - m->loc, (*hit)->normal) / (FVector::DotProduct(reflection, (*hit)->normal));
				locBounce = m->loc + reflection*t;

				// find return vector
				// TODO: See if any change in accuracy in just using the hit version, should be pretty close angles

				// find ray return
				returnRay = reflect((*hit)->normal, -reflection);

				// find spherical location
				Octree bounce(locBounce, m->size);
				inRange(&bounce);
				bounce.locSpherical.X += m->locSpherical.X + FVector::Dist(bounce.loc, m->loc);
				bounce.locSpherical.X /= 2;

				// Convert to contribution index
				m->idx.X = (int32)((bounce.locSpherical.X - MinRange) / RangeRes);
				m->idx.Y = (int32)((bounce.locSpherical.Y - minAzimuth)/ AzimuthRes);
				m->val = FVector::DotProduct(returnRay, (*hit)->normalImpact);
				m->val *= (m->z - z_water) / (m->z + z_water);
				m->val *= ((*hit)->z - z_water) / ((*hit)->z + z_water);

				// DrawDebugPoint(GetWorld(), m->loc, 5, FColor::Red, false, DeltaTime*TicksPerCapture);
				// DrawDebugPoint(GetWorld(), bounce.loc, 5, FColor::Blue, false, DeltaTime*TicksPerCapture);
			}
		}, false);
		UE_LOG(LogHolodeck, Warning, TEXT("MULTIPATH : %f"), c.CalcMs());

		// ADD IN MULTIPATH CONTRIBUTIONS
		for(TArray<Octree*>& bin : cluster){
			for(Octree* l : bin){
				for(int i=FGenericPlatformMath::Max(0,l->idx.X-extraContr); i<FGenericPlatformMath::Min(BinsRange,l->idx.X+extraContr+1); i++){
					for(int j=FGenericPlatformMath::Max(0,l->idx.Y-extraContr); j<FGenericPlatformMath::Min(BinsAzimuth,l->idx.Y+extraContr+1); j++){
						idx = i*BinsAzimuth + j;
						result[idx] += l->val;
						++count[idx];
					}
				}
			}
		}

		UE_LOG(LogHolodeck, Warning, TEXT("ADD IN MP CONTRI : %f"), c.CalcMs());
		UE_LOG(LogHolodeck, Warning, TEXT("TOTAL : %f"), b.CalcMs());


		// MOVE THEM INTO BUFFER
		for (int i = 0; i < BinsRange*BinsAzimuth; i++) {
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
			DrawDebugLine(GetWorld(), spherToEuc(MinRange, minAzimuth, minElev, tran), spherToEuc(MaxRange, minAzimuth, minElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, debugThickness);
			DrawDebugLine(GetWorld(), spherToEuc(MinRange, minAzimuth, maxElev, tran), spherToEuc(MaxRange, minAzimuth, maxElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, debugThickness);
			DrawDebugLine(GetWorld(), spherToEuc(MinRange, maxAzimuth, minElev, tran), spherToEuc(MaxRange, maxAzimuth, minElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, debugThickness);
			DrawDebugLine(GetWorld(), spherToEuc(MinRange, maxAzimuth, maxElev, tran), spherToEuc(MaxRange, maxAzimuth, maxElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, debugThickness);

			// azimuth lines (should be arcs, we're being lazy)
			DrawDebugLine(GetWorld(), spherToEuc(MinRange, minAzimuth, minElev, tran), spherToEuc(MinRange, maxAzimuth, minElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, debugThickness);
			DrawDebugLine(GetWorld(), spherToEuc(MinRange, minAzimuth, maxElev, tran), spherToEuc(MinRange, maxAzimuth, maxElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, debugThickness);
			DrawDebugLine(GetWorld(), spherToEuc(MaxRange, minAzimuth, minElev, tran), spherToEuc(MaxRange, maxAzimuth, minElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, debugThickness);
			DrawDebugLine(GetWorld(), spherToEuc(MaxRange, minAzimuth, maxElev, tran), spherToEuc(MaxRange, maxAzimuth, maxElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, debugThickness);

			// elevation lines (should be arcs, we're being lazy)
			DrawDebugLine(GetWorld(), spherToEuc(MinRange, minAzimuth, minElev, tran), spherToEuc(MinRange, minAzimuth, maxElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, debugThickness);
			DrawDebugLine(GetWorld(), spherToEuc(MinRange, maxAzimuth, minElev, tran), spherToEuc(MinRange, maxAzimuth, maxElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, debugThickness);
			DrawDebugLine(GetWorld(), spherToEuc(MaxRange, minAzimuth, minElev, tran), spherToEuc(MaxRange, minAzimuth, maxElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, debugThickness);
			DrawDebugLine(GetWorld(), spherToEuc(MaxRange, maxAzimuth, minElev, tran), spherToEuc(MaxRange, maxAzimuth, maxElev, tran), FColor::Green, false, DeltaTime*TicksPerCapture, ECC_WorldStatic, debugThickness);
		}		
	}
}
