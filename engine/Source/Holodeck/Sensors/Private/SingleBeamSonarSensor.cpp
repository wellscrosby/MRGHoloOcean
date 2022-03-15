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
	
	BinsRange = 200;
	BinsCentralAngle = 6;
	BinsOpeningAngle = 5;

	// range in cm
	MinRange = 50;
	MaxRange = 1000;

	Super::ParseSensorParms(ParmsJson);

	// user can override default values
	TSharedPtr<FJsonObject> JsonParsed;
	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(ParmsJson);
	if (FJsonSerializer::Deserialize(JsonReader, JsonParsed)) {
		if (JsonParsed->HasTypedField<EJson::Number>("BinsRange")) {
			BinsRange = JsonParsed->GetIntegerField("BinsRange");
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
		if (JsonParsed->HasTypedField<EJson::Number>("MaxRange")) {
			MaxRange = JsonParsed->GetNumberField("MaxRange")*100;
		}
		if (JsonParsed->HasTypedField<EJson::Number>("MinRange")) {
			MinRange = JsonParsed->GetNumberField("MinRange")*100;
		}
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
		if (JsonParsed->HasTypedField<EJson::Boolean>("ViewRegion")) {
			ViewRegion = JsonParsed->GetBoolField("ViewRegion");
		}
		if (JsonParsed->HasTypedField<EJson::Number>("ViewOctree")) {
			ViewOctree = JsonParsed->GetIntegerField("ViewOctree");
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
	RangeRes = (MaxRange - MinRange) / BinsRange;
	CentralAngleRes = CentralAngle / BinsCentralAngle;
	OpeningAngleRes = OpeningAngle / BinsOpeningAngle;
	
	// setup count of each bin
	count = new int32[BinsRange]();

	for(int i=0;i<BinsCentralAngle*BinsOpeningAngle;i++){
		sortedLeaves.Add(TArray<Octree*>());
		sortedLeaves[i].Reserve(10000);
	}
}


// fast arctan function
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

// determine if a single leaf is in your tree
bool USingleBeamSonarSensor::inRange(Octree* tree){
	FTransform SensortoWorld = this->GetComponentTransform();
	// if it's not a leaf, we use a bigger search area
	float offset = 0;
	float radius = 0;

	sqrt2 = UKismetMathLibrary::Sqrt(3)/2;
	sinOffset = UKismetMathLibrary::DegSin(FGenericPlatformMath::Min(CentralAngle, OpeningAngle)/2);

	if(tree->size != Octree::OctreeMin){
		radius = tree->size*sqrt2;
		offset = radius/sinOffset;
		SensortoWorld.AddToTranslation( -this->GetForwardVector()*offset );
	}
	
	// transform location to sensor frame instead of global (x y z)
	FVector locLocal = SensortoWorld.GetRotation().UnrotateVector(tree->loc-SensortoWorld.GetTranslation());

	// check if it's in range
	tree->locSpherical.X = locLocal.Size();
	if(MinRange+offset-radius >= tree->locSpherical.X || tree->locSpherical.X >= MaxRange+offset+radius) return false; 

	// check if OpeningAngle is in range. OpeningAngle is angle off of x-axis
	tree->locSpherical.Z = ATan2ApproxSingleBeam(UKismetMathLibrary::Sqrt(UKismetMathLibrary::Square(locLocal.Y)+UKismetMathLibrary::Square(locLocal.Z)), locLocal.X); //OpeningAngle of leaf we are inspecting
	if(minOpeningAngle >= tree->locSpherical.Z || tree->locSpherical.Z >= maxOpeningAngle) return false;

	// save CentralAngle for shadowing later. CentralAngle goes around the x-axis
	tree->locSpherical.Y = ATan2ApproxSingleBeam(locLocal.Z, locLocal.Y);

	// otherwise it's in!
	return true;
}	

// leavesInRange recursively check if each tree and then children etc are in range
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

// findLeaves used for parallizing
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
		std::fill(result, result+BinsRange, 0);
		std::fill(count, count+BinsRange, 0);
		
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
		float eps = 100;
		ParallelFor(BinsCentralAngle*BinsOpeningAngle, [&](int32 i){
			TArray<Octree*>& binLeafs = sortedLeaves.GetData()[i]; 

			// sort from closest to farthest
			binLeafs.Sort([](const Octree& a, const Octree& b){
				// range
				return a.locSpherical.X < b.locSpherical.X;
			});

			// Get the closest cluster in the bin
			// location of each bin irange, icentralangle, iOpeningAngle
			int32 idx;
			float diff, z_t, R;
			float z_i = sos_water * density_water;
			Octree* jth;
			for(int j=0;j<binLeafs.Num()-1;j++){
				jth = binLeafs.GetData()[j];
				// add noise to range measurements
				double range_noise = rNoise.sampleExponential();
				jth->idx.X = (int32)((jth->locSpherical.X - MinRange + range_noise) / RangeRes); 
				idx = jth->idx.X;
				z_t = jth->sos * jth->density;
				R = (z_t - z_i) / (z_t + z_i);

				// calculate intesity and add them up
				result[idx] += jth->val * R; 
				++count[idx];
				
				// Light up some bins to visualize things. 
				// Make sure you change the bool at end of ParallelFor to true to turn off running in parallel, since 
				// this can't be done in parallel
				// DrawDebugPoint(GetWorld(), jth->loc, 5, FColor::Red, false, DeltaTime*TicksPerCapture);

				diff = FMath::Abs(jth->locSpherical.X - binLeafs.GetData()[j+1]->locSpherical.X);
				if(diff > eps) break;
			}
		}, true);

		
		// MOVE THEM INTO BUFFER
		for (int i = 0; i < BinsRange; i++) {
			if(count[i] != 0){
				// actually take the average of the intensities
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

			DrawDebugCone(GetWorld(), GetComponentLocation(), GetForwardVector(), length, (OpeningAngle/2)*Pi/180, (OpeningAngle/2)*Pi/180, DebugNumSides, FColor::Green, false, .00, ECC_WorldStatic, debugThickness);
		}		
	}
}
