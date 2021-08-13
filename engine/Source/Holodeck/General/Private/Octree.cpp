// Fill out your copyright notice in the Description page of Project Settings.


#include "Octree.h"

// Initialize static variables
TArray<FVector> Octree::corners = {FVector( 1,  1, 1),
                                    FVector( 1,  1, -1),
                                    FVector( 1, -1,  1),
                                    FVector( 1, -1, -1),
                                    FVector(-1,  1,  1),
                                    FVector(-1,  1, -1),
                                    FVector(-1, -1,  1),
                                    FVector(-1, -1, -1)};
FVector Octree::offset = 10*FVector(KINDA_SMALL_NUMBER, KINDA_SMALL_NUMBER, KINDA_SMALL_NUMBER) / sqrt(2.9);
float Octree::cornerSize = 0.1;
FCollisionQueryParams Octree::params = Octree::init_params();
float Octree::OctreeMax;
float Octree::OctreeMin;
FVector Octree::EnvMin;
FVector Octree::EnvMax;
UWorld* Octree::World;

float sign(float val){
    bool s = signbit(val);
    if(s) return -1.0;
    else return 1.0;
}

void Octree::initOctree(float min, float max, FVector eMin, FVector eMax){
    OctreeMin = min;
    OctreeMax = max;
    EnvMin = eMin;
    EnvMax = eMax;
}

TArray<Octree*> Octree::getOctreeRoots(UWorld* w){
    TArray<Octree*> octree;
    World = w;

    // Get caching/loading location
    FString filePath = FPaths::ProjectDir() + "Octrees/" + World->GetMapName();
    filePath += "/" + FString::FromInt(OctreeMin) + "_" + FString::FromInt(OctreeMax);
    FString rootFile = filePath + "/" + "root.csv";

    // if we've already saved what root nodes should be, open 'em up
    if(FPaths::FileExists(rootFile)){
        UE_LOG(LogHolodeck, Log, TEXT("HolodeckServer::Loading Octree roots.."));
        // Get ready
        FString fileData;
        TArray<FString> lines;
        TArray<FString> data;
        FVector center;

        // Load data
        FFileHelper::LoadFileToString(fileData, *rootFile);
        fileData.ParseIntoArray(lines, TEXT("\n"), true);
        for(FString l : lines){
            l.ParseIntoArray(data, TEXT(" "), true);
            center = FVector(FCString::Atof(*data[0]), FCString::Atof(*data[1]), FCString::Atof(*data[2]));
            FString filename = filePath + "/" + FString::FromInt((int)center.X) + "_" 
                                            + FString::FromInt((int)center.Y) + "_" 
                                            + FString::FromInt((int)center.Z) + ".json";
            Octree* o = new Octree(center, OctreeMax, filename);
            octree.Add(o);
        }
    }
    // otherwise figure it out!
    else{
        UE_LOG(LogHolodeck, Log, TEXT("HolodeckServer::Setting up Octree roots.."));
        // Otherwise, make the octrees
        FIntVector nCells = FIntVector((EnvMax - EnvMin) / OctreeMax) + FIntVector(1);
        int32 total = nCells.X * nCells.Y * nCells.Z;
        for(int32 i = 0; i < nCells.X; i++) {
            for(int32 j = 0; j < nCells.Y; j++) {
                for(int32 k = 0; k < nCells.Z; k++) {
                    FVector center = FVector(i*OctreeMax, j*OctreeMax, k*OctreeMax) + EnvMin;
                    Octree* o = Octree::newHeadOctree(center, OctreeMax, filePath);
                    if(o){
                        octree.Add(o);
                    }

                    float percent = 100.0*(i*nCells.Y*nCells.Z + j*nCells.Z + k)/total; 
                    UE_LOG(LogHolodeck, Log, TEXT("Creating Octree %f"), percent); 
                }
            }
        }

        // Save these, so we don't have to figure it out again
        FString root = "";
        for(Octree* o : octree){
            root += FString::FromInt((int)o->loc.X)
                        + " " + FString::FromInt((int)o->loc.Y)
                        + " " + FString::FromInt((int)o->loc.Z)
                        + "\n";
        }
        FFileHelper::SaveStringToFile(root, *rootFile);
    }

    return octree;
}

Octree* Octree::newHeadOctree(FVector center, float octreeSize, FString filePath, FString actorName){
    // CHECK IF FILE ALREADY EXISTS, IF IT DOES THIS THERE'S A VALID OCTREE
    FString filename = filePath + "/" + FString::FromInt((int)center.X) + "_" 
                                    + FString::FromInt((int)center.Y) + "_" 
                                    + FString::FromInt((int)center.Z) + ".json";
    if(FPaths::FileExists(filename)) return new Octree(center, octreeSize, filename);
    
    // IF IT DOESN'T, CHECK IF THERE'S SOMETHING THERE
    FHitResult hit = FHitResult();
    bool occup = World->SweepSingleByChannel(hit, center, center+offset, FQuat::Identity, ECollisionChannel::ECC_WorldStatic, FCollisionShape::MakeBox(FVector(octreeSize/2)), params);
    occup = hit.bStartPenetrating;
    // if we're making for an actor, make sure we're hitting it and not something else
    if(occup && actorName != "" && actorName != hit.GetActor()->GetName()){
        occup = false;
    }
    if(occup){
        // Check if it's full
        bool full = true;
        // check to see if each corner is overlapping
        float distToCorner = octreeSize/2 - cornerSize;
        for(FVector corner : corners){
            full = World->OverlapBlockingTestByChannel(center+(corner*distToCorner), FQuat::Identity, ECollisionChannel::ECC_Pawn, FCollisionShape::MakeBox(FVector(cornerSize)), params);
            if(!full){
                return new Octree(center, octreeSize, filename);
            }
        }
    }

    // IF THERE ISN'T, RETURN NULL
    return nullptr;

}

void Octree::makeOctree(FVector center, float octreeSize, TArray<Octree*>& parent, FString actorName){
    /*
    * There's a bug in UE4 4.22 where if you're sweep has length less than KINDA_SMALL_NUMBER it doesn't do anything
    * https://answers.unrealengine.com/questions/887018/422-spheretraceforobjects-node-is-not-working-anym.html
    * This was fixed in 4.24, but until we update to it, we offset the end by the smallest # possible, and use bFindInitialOverlaps, bStartPenetrating to make sure we only get overlaps at the start.
    * This shouldn't effect octree generation speed if there's an overlap, but may make empty searches a smidge slower.
    */
    FHitResult hit = FHitResult();
    bool occup = World->SweepSingleByChannel(hit, center, center+offset, FQuat::Identity, ECollisionChannel::ECC_WorldStatic, FCollisionShape::MakeBox(FVector(octreeSize/2)), params);
    occup = hit.bStartPenetrating;
    // if we're making for an actor, make sure we're hitting it and not something else
    if(occup && actorName != "" && actorName != hit.GetActor()->GetName()){
        occup = false;
    }

    // if it's occupied
	if(occup){
        // Check if it's full
        bool full = true;
        // check to see if each corner is overlapping
        float distToCorner = octreeSize/2 - cornerSize;
        for(FVector corner : corners){
            full = World->OverlapBlockingTestByChannel(center+(corner*distToCorner), FQuat::Identity, ECollisionChannel::ECC_Pawn, FCollisionShape::MakeBox(FVector(cornerSize)), params);
            if(!full) break;
        }

        if(!full){
            // make a tree to insert
            Octree* child = new Octree(center, octreeSize);
            
            // if it still needs to be broken down, iterate through corners
            if(octreeSize > OctreeMin){
                for(FVector off : corners){
                    makeOctree(center+(off*octreeSize/4), octreeSize/2, child->leafs, actorName);
                }
            }

            // if it's all the way broken down, save the normal
            else{
                child->normal = hit.Normal;

                // clean normal
                if(isnan(child->normal.X)) child->normal.X = sign(child->normal.X); 
                if(isnan(child->normal.Y)) child->normal.Y = sign(child->normal.Y); 
                if(isnan(child->normal.Z)) child->normal.Z = sign(child->normal.Z); 
                if(hit.Normal.ContainsNaN()){
                    UE_LOG(LogHolodeck, Warning, TEXT("Found position: %s"), *child->loc.ToString());
                    UE_LOG(LogHolodeck, Warning, TEXT("Found nan: %s"), *child->normal.ToString());
                }
                // DrawDebugLine(World, center, center+hit.Normal*OctreeMin/2, FColor::Blue, true, 100, ECC_WorldStatic, 1.f);
                // DrawDebugBox(World, center, FVector(size/2), FColor::Green, true, 2, ECC_WorldStatic, .5f);
            }

            parent.Add(child);
        }
	}

    // DrawDebugBox(World, center, FVector(size/2), FColor::Red, true, 2, ECC_WorldStatic, .5f);
}

int Octree::numLeafs(){
    if(leafs.Num()==0){
        return 1;
    }
    else{
        int num = 1;
        for(Octree* leaf : leafs){
            num += leaf->numLeafs();
        }
        return num;
    }
}

void Octree::toJson(){
    // make directory
    FFileManagerGeneric().MakeDirectory(*FPaths::GetPath(file), true);

    // calculate buffer size and make writer
    int num = numLeafs()*100;
    char* buffer = new char[num]();
    gason::JSonBuilder doc(buffer, num-1);

    // fill in buffer
    toJson(doc);

    if( doc.isBufferAdequate() ){
        // String to file
        FILE* fp = fopen(TCHAR_TO_ANSI(*file), "w+t");
        fwrite(buffer, strlen(buffer), 1, fp);
        fclose(fp);
    }
    else{
        UE_LOG(LogHolodeck, Warning, TEXT("Octree: The buffer is too small and the output json for file %s is not valid."), *file);
    }

    delete[] buffer;
}

void Octree::toJson(gason::JSonBuilder& doc){
    doc.startObject()
        .startArray("p")
            .addValue((int)loc[0])
            .addValue((int)loc[1])
            .addValue((int)loc[2])
        .endArray();

    if(size != OctreeMin){
        doc.startArray("l");
        for(Octree* l : leafs){
            l->toJson(doc);
        }
        doc.endArray();
    }
    else{
        doc.startArray("n")
                .addValue(normal[0])
                .addValue(normal[1])
                .addValue(normal[2])
            .endArray();
    }

    doc.endObject();
}

void Octree::load(){
    // if it's not already loaded
    if(leafs.Num() == 0){
        // if it's been saved as a json, load it
        if(FPaths::FileExists(file)){
            UE_LOG(LogHolodeck, Log, TEXT("Loading Octree %s"), *file);
            // load file to a string
            gason::JsonAllocator allocator;
            std::ifstream t(TCHAR_TO_ANSI(*file));
            std::string str((std::istreambuf_iterator<char>(t)),
                            std::istreambuf_iterator<char>());
            char* source = &str[0];

            // process json
            char* endptr;
            gason::JsonValue json;
            int status = gason::jsonParse(source, &endptr, &json, allocator);

            // load in leafs
            for(gason::JsonNode* o : json){
                if(o->key[0] == 'l'){
                    for(gason::JsonNode* l : o->value){
                        loadJson(l->value, leafs, size/2);
                    }
                }
            }
        }

        // Otherwise build it & save for later
        else{
            UE_LOG(LogHolodeck, Log, TEXT("Making Octree %s"), *file);
            for(FVector off : corners){
                makeOctree(loc+(off*size/4), size/2, leafs);
            }
            toJson();
        }

    }
}

void Octree::loadJson(gason::JsonValue& json, TArray<Octree*>& parent, float size){
    Octree* child = new Octree;
    for(gason::JsonNode* o : json){
        if(o->key[0] == 'p'){
            gason::JsonNode* arr = o->value.toNode();
            child->loc = FVector(arr->value.toNumber(), arr->next->value.toNumber(), arr->next->next->value.toNumber());
        }
        if(o->key[0] == 'l'){
            for(gason::JsonNode* l : o->value){
                loadJson(l->value, child->leafs, size/2);
            }
        }
        if(o->key[0] == 'n'){
            gason::JsonNode* arr = o->value.toNode();
            child->normal = FVector(arr->value.toNumber(), arr->next->value.toNumber(), arr->next->next->value.toNumber());
        }
    }
    child->size = size;
    parent.Add(child);
}
