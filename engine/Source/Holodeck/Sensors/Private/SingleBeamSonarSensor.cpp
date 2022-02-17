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

	// default values, user can override through ParmsJson
	Azimuth = 360;
	Elevation = 5;
	// BinsRange = 300;
	// BinsAzimuth = 6;
	// BinsElevation = 1

	Super::ParseSensorParms(ParmsJson);

	TSharedPtr<FJsonObject> JsonParsed;
	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(ParmsJson);
	if (FJsonSerializer::Deserialize(JsonReader, JsonParsed)) {

		// For handling noise. What is this doing?
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

		if (JsonParsed->HasTypedField<EJson::Boolean>("ViewRegion")) {
			ViewRegion = JsonParsed->GetBoolField("ViewRegion");
		}

		if (JsonParsed->HasTypedField<EJson::Number>("ViewOctree")) {
			ViewOctree = JsonParsed->GetIntegerField("ViewOctree");
		}
		if (JsonParsed->HasTypedField<EJson::Number>("RangeSigma")) {
			rNoise.initSigma(JsonParsed->GetNumberField("RangeSigma")*100);
		}
		if (JsonParsed->HasTypedField<EJson::Number>("RangeCov")) {
			rNoise.initCov(JsonParsed->GetNumberField("RangeCov")*100*100);
		}

	}
	else {
		UE_LOG(LogHolodeck, Fatal, TEXT("USingleBeamSonarSensor::ParseSensorParms:: Unable to parse json."));
	}

	if(BinsElevation == 0){
		BinsElevation = Elevation*10;
	}
}

void USingleBeamSonarSensor::InitializeSensor() {
	Super::InitializeSensor();
	
	// minAzimuth = -180;
	// maxAzimuth = 180;
	minElev = 0;
	maxElev = Elevation/2;

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
}

FVector spherToEucSingleBeamSingleBeam(float r, float theta, float phi, FTransform SensortoWorld){
	float x = r*UKismetMathLibrary::DegSin(phi)*UKismetMathLibrary::DegCos(theta);
	float y = r*UKismetMathLibrary::DegSin(phi)*UKismetMathLibrary::DegSin(theta);
	float z = r*UKismetMathLibrary::DegCos(phi);
	return UKismetMathLibrary::TransformLocation(SensortoWorld, FVector(x, y, z));
}

float ATan2ApproxSingleBeam(float y, float x){
    //http://pubs.opengroup.org/onlinepubs/009695399/functions/atan2.html
    //Volkan SALMA

    const float ONEQTR_PI = Pi / 4.0;
	const float THRQTR_PI = 3.0 * Pi / 4.0;
	float r, angle;
	float abs_y = fabs(y) + 1e-10f;      // kludge to prevent 0/0 condition
	if ( x < 0.0f )
	{
		r = (x + abs_y) / (abs_y - x);
		angle = THRQTR_PI;
	}
	else
	{
		r = (x - abs_y) / (x + abs_y);
		angle = ONEQTR_PI;
	}
	angle += (0.1963f * r * r - 0.9817f) * r;
	angle *= 180/Pi;
	if ( y < 0.0f )
		return( -angle );     // negate if in quad III or IV
	else
		return( angle );
}

// inRange tells if a single leaf is in your tree
bool USingleBeamSonarSensor::inRange(Octree* tree){
	FTransform SensortoWorld = this->GetComponentTransform();
	// if it's not a leaf, we use a bigger search area
	float offset = 0;
	float radius = 0;

	sqrt2 = UKismetMathLibrary::Sqrt(2);
	sinOffset = UKismetMathLibrary::DegSin(FGenericPlatformMath::Min(Azimuth, Elevation)/2);

	if(tree->size != Octree::OctreeMin){
		radius = tree->size/sqrt2;
		offset = radius/sinOffset;
		SensortoWorld.AddToTranslation( -this->GetForwardVector()*offset );
	}
	
	// --------------------------------------------------------]

	// transform location to sensor frame instead of global (x y z)
	// FVector locLocal = SensortoWorld.InverseTransformPositionNoScale(tree->loc); 
	FVector locLocal = SensortoWorld.GetRotation().UnrotateVector(tree->loc-SensortoWorld.GetTranslation());

	// check if it's in range
	tree->locSpherical.X = locLocal.Size() + rNoise.sampleFloat();
	if(MinRange+offset-radius >= tree->locSpherical.X || tree->locSpherical.X >= MaxRange+offset+radius) return false; 

	// check if azimuth is in
	tree->locSpherical.Y = ATan2ApproxSingleBeam(locLocal.Z, locLocal.Y) + aziNoise.sampleFloat();
	// tree->locSpherical.Y = ATan2ApproxSingleBeam(-locLocal.Y, locLocal.X) + aziNoise.sampleFloat();
	// if(minAzimuth >= tree->locSpherical.Y || tree->locSpherical.Y >= maxAzimuth) return false;

	// check if "elevation" is in
	// tree->locSpherical.Z = ATan2ApproxSingleBeam(locLocal.Size2D(), locLocal.Z);
	tree->locSpherical.Z = ATan2ApproxSingleBeam(UKismetMathLibrary::Sqrt(UKismetMathLibrary::Square(locLocal.Y)+UKismetMathLibrary::Square(locLocal.Z)), locLocal.X); //elevation of leaf we are inspecting
	if(minElev >= tree->locSpherical.Z || tree->locSpherical.Z >= maxElev) return false;

	// // check if it's in range. 
	// tree->locSpherical.X = locLocal.Size() + rNoise.sampleFloat();   // range of leaf we are inspecting
	// if(MinRange+offset-radius >= tree->locSpherical.X || tree->locSpherical.X >= MaxRange+offset+radius) return false; 

	// //check if radius is in
	// tree->locSpherical.Z = ATan2ApproxSingleBeam(UKismetMathLibrary::Sqrt(UKismetMathLibrary::Square(locLocal.Y)+UKismetMathLibrary::Square(locLocal.Z)), locLocal.X); //elevation of leaf we are inspecting
	// float distance = tree->locSpherical.X*UKismetMathLibrary::DegCos(tree->locSpherical.Z);
	// float radius_leaf = tree->locSpherical.X*UKismetMathLibrary::DegSin(tree->locSpherical.Z);
	// float radius_max = distance*UKismetMathLibrary::DegTan(Elevation/2);
	// if(radius_leaf > radius_max) return false;

	// // get "azimuth"
	// tree->locSpherical.Y = ATan2ApproxSingleBeam(locLocal.Z, locLocal.Y) + aziNoise.sampleFloat();
	// if(tree->locSpherical.Y < -180.0 || tree->locSpherical.Y > 0) return false;

	// test azimuth bounds


	// if(minAzimuth >= tree->locSpherical.Y || tree->locSpherical.Y >= maxAzimuth) return false;

	// // // check if azimuth is in
	// tree->locSpherical.Y = ATan2ApproxSingleBeam(-locLocal.Y, locLocal.X) + aziNoise.sampleFloat();
	// if(minAzimuth >= tree->locSpherical.Y || tree->locSpherical.Y >= maxAzimuth) return false;

	// check if "elevation" is in circle radius
	// tree->locSpherical.Z = ATan2ApproxSingleBeam(locLocal.Size2D(), locLocal.Z);
	// if(minElev >= tree->locSpherical.Z || tree->locSpherical.Z >= maxElev) return false;
	
	// otherwise it's in!
	return true;
}	

// leavesInRange recursively check if each tree and then children etc are in range. will probs need to call my own inRange func
void USingleBeamSonarSensor::leavesInRange(Octree* tree, TArray<Octree*>& rLeaves, float stopAt){
	bool in = inRange(tree);
	if(in){
		if(tree->size == stopAt){
			if(stopAt == Octree::OctreeMin){
				// Compute contribution while we're parallelized
				// If no contribution, we don't have to add it in
				FVector normalImpact = GetComponentLocation() - tree->loc; 
				normalImpact.Normalize();

				// compute contribution
				float val = FVector::DotProduct(tree->normal, normalImpact);
				if(val > 0){
					tree->val = val;
					rLeaves.Add(tree);
				} 
			}
			else{
				rLeaves.Add(tree);
			}
			return;
		}

		for(Octree* l : tree->leaves){
			leavesInRange(l, rLeaves, stopAt);
		}
	}
	else if(tree->size >= Octree::OctreeMax){
		tree->unload();
	}
}

// findLeaves used for parallizing. will need to call new leavesInRange
void USingleBeamSonarSensor::findLeaves(){
	// Empty everything out
	bigLeaves.Reset();
	for(auto& fl: foundLeaves){
		fl.Reset();
	}

	// FILTER TO GET THE bigLeaves WE WANT
	// start at octree root, store in bigLeaves
	leavesInRange(octree, bigLeaves, Octree::OctreeMax);
	bigLeaves += agents;

	ParallelFor(bigLeaves.Num(), [&](int32 i){
		Octree* leaf = bigLeaves.GetData()[i];
		leaf->load();
		for(Octree* l : leaf->leaves)
			leavesInRange(l, foundLeaves.GetData()[i%1000], Octree::OctreeMin);
	});
}

void USingleBeamSonarSensor::TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	Super::TickSensorComponent(DeltaTime, TickType, ThisTickFunction);

	if(TickCounter == 0){
		// reset things and get ready
		float* result = static_cast<float*>(Buffer);
		std::fill(result, result+BinsRange*BinsAzimuth, 0);
		std::fill(count, count+BinsRange*BinsAzimuth, 0);
		
		for(auto& sl: sortedLeaves){
			sl.Reset();
		}

		// Finds leaves in range and puts them in foundLeaves
		findLeaves();		// does not return anything, saves to foundLeaves


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
				// array of arrays (the rectangle we split off)
				sortedLeaves[idx].Emplace(l);
			}
		}

		// UE_LOG(LogTemp, Warning, TEXT("Numcounts: %d"), counter);



		// HANDLE SHADOWING
		float eps = 16;
		ParallelFor(BinsAzimuth*BinsElevation, [&](int32 i){
			TArray<Octree*>& binLeafs = sortedLeaves.GetData()[i]; 

			// sort from closest to farthest
			binLeafs.Sort([](const Octree& a, const Octree& b){
				// range
				// locSpherical: xrange, azimuth, elevation
				//                x         y        z
				return a.locSpherical.X < b.locSpherical.X;
			});

			// Get the closest cluster in the bin
			// location of each bin irange, iazimuth, ielevation
			int32 idx;
			float diff, z_t, R;
			float z_i = sos_water * density_water;
			Octree* jth;
			for(int j=0;j<binLeafs.Num()-1;j++){
				jth = binLeafs.GetData()[j];
				
				jth->idx.X = (int32)((jth->locSpherical.X - MinRange) / RangeRes);
				idx = jth->idx.X*BinsAzimuth + jth->idx.Y;
				z_t = jth->sos * jth->density;
				R = (z_t - z_i) / (z_t + z_i);

				// calculate intesity and add them up
				result[idx] += jth->val * R;
				// number of 
				++count[idx];
				
				// diff = FVector::Dist(jth->loc, binLeafs.GetData()[j+1]->loc);
				diff = FMath::Abs(jth->locSpherical.X - binLeafs.GetData()[j+1]->locSpherical.X);
				if(diff > eps) break;
			}

		});

		
		// MOVE THEM INTO BUFFER
		for (int i = 0; i < BinsRange*BinsAzimuth; i++) {
			if(count[i] != 0){
				// actually take the average
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
			float DebugNumSides = 6; //change later?
			float length = (MaxRange - MinRange)*100; //length of cone in cm
			// FVector direction = UKismetMathLibrary::GetDirectionUnitVector(spherToEucSingleBeamSingleBeam(MinRange, -Azimuth/2, 90 - Elevation/2, tran), spherToEucSingleBeamSingleBeam(MaxRange, Azimuth/2, 90 + Elevation/2, tran));
// spherToEucSingleBeamSingleBeam(MinRange, -Azimuth/2, 90 - Elevation/2, tran)
			DrawDebugCone(GetWorld(), GetComponentLocation(), GetForwardVector(), length, (Elevation/2)*Pi/180, (Elevation/2)*Pi/180, DebugNumSides, FColor::Green, false, .00, ECC_WorldStatic, debugThickness);
		}		
	}
}
