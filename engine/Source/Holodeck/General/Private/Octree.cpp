// Fill out your copyright notice in the Description page of Project Settings.


#include "Octree.h"

// Initialize static variables
// Used when making octree
TArray<FVector> Octree::corners = {FVector( 1,  1, 1),
                                    FVector( 1,  1, -1),
                                    FVector( 1, -1,  1),
                                    FVector( 1, -1, -1),
                                    FVector(-1,  1,  1),
                                    FVector(-1,  1, -1),
                                    FVector(-1, -1,  1),
                                    FVector(-1, -1, -1)};
TArray<FVector> Octree::sides = {FVector( 0, 0, 1),
                                    FVector( 0, 0,-1),
                                    FVector( 0, 1, 0),
                                    FVector( 0,-1, 0),
                                    FVector( 1, 0, 0),
                                    FVector(-1, 0, 0)};
FVector Octree::offset = 10*FVector(KINDA_SMALL_NUMBER, KINDA_SMALL_NUMBER, KINDA_SMALL_NUMBER) / sqrt(2.9);
float Octree::cornerSize = 0.01;
FCollisionQueryParams Octree::params = Octree::init_params();

// Misc constants
float Octree::OctreeRoot;
float Octree::OctreeMax;
float Octree::OctreeMin;
FVector Octree::EnvMin;
FVector Octree::EnvMax;
FVector Octree::EnvCenter;
UWorld* Octree::World;

float sign(float val){
    bool s = signbit(val);
    if(s) return -1.0;
    else return 1.0;
}

void Octree::initOctree(UWorld* w){
    World = w;

    // Load environment size
    if (!FParse::Value(FCommandLine::Get(), TEXT("EnvMinX="), EnvMin.X)) EnvMin.X = -10;
    if (!FParse::Value(FCommandLine::Get(), TEXT("EnvMinY="), EnvMin.Y)) EnvMin.Y = -10;
    if (!FParse::Value(FCommandLine::Get(), TEXT("EnvMinZ="), EnvMin.Z)) EnvMin.Z = -10;
    if (!FParse::Value(FCommandLine::Get(), TEXT("EnvMaxX="), EnvMax.X)) EnvMax.X = 10;
    if (!FParse::Value(FCommandLine::Get(), TEXT("EnvMaxY="), EnvMax.Y)) EnvMax.Y = 10;
    if (!FParse::Value(FCommandLine::Get(), TEXT("EnvMaxZ="), EnvMax.Z)) EnvMax.Z = 10;
    // Clean environment size
    EnvMin = ConvertLinearVector(EnvMin, ClientToUE);
    EnvMax = ConvertLinearVector(EnvMax, ClientToUE);
    FVector min = FVector((int)FGenericPlatformMath::Min(EnvMin.X, EnvMax.X), (int)FGenericPlatformMath::Min(EnvMin.Y, EnvMax.Y), (int)FGenericPlatformMath::Min(EnvMin.Z, EnvMax.Z));
    FVector max = FVector((int)FGenericPlatformMath::Max(EnvMin.X, EnvMax.X), (int)FGenericPlatformMath::Max(EnvMin.Y, EnvMax.Y), (int)FGenericPlatformMath::Max(EnvMin.Z, EnvMax.Z));
    EnvMin = min;
    EnvMax = max;
    UE_LOG(LogHolodeck, Log, TEXT("Octree:: EnvMin: %s"), *EnvMin.ToString());
    UE_LOG(LogHolodeck, Log, TEXT("Octree:: EnvMax: %s"), *EnvMax.ToString());

    // Get octree min/max
    float tempVal;
    if (!FParse::Value(FCommandLine::Get(), TEXT("OctreeMin="), tempVal)) tempVal = .1;
    OctreeMin = (tempVal*100);
    if (!FParse::Value(FCommandLine::Get(), TEXT("OctreeMax="), tempVal)) tempVal = 5;
    OctreeMax = (tempVal*100);

    // Calculate where/how big is biggest octree
    EnvCenter = (EnvMax + EnvMin) / 2;
    OctreeRoot = (EnvMax - EnvMin).GetAbsMax();

    // Make max/root a multiple of min
    tempVal = OctreeMin;
    while(tempVal <= OctreeMax){
        tempVal *= 2;
    }
    OctreeMax = tempVal;
    while(tempVal <= OctreeRoot){
        tempVal *= 2;
    }
    OctreeRoot = tempVal;
    UE_LOG(LogHolodeck, Log, TEXT("Octree:: OctreeMin: %f, OctreeMax: %f, OctreeRoot: %f"), OctreeMin, OctreeMax, OctreeRoot);
}

Octree* Octree::makeEnvOctreeRoot(){
    // Get caching/loading location
    FString filePath = FPaths::ProjectDir() + "Octrees/" + World->GetMapName();
    filePath += "/min" + FString::FromInt(OctreeMin) + "_max" + FString::FromInt(OctreeMax);
    FString rootFile = filePath + "/" + "roots.json";

    // load
    UE_LOG(LogHolodeck, Log, TEXT("Octree::Making Octree root"));
    Octree* root = new Octree(EnvCenter, OctreeRoot, rootFile);
    root->makeTill = Octree::OctreeMax;
    root->load();
    
    // set filename/makeTill for all OctreeMax nodes
    std::function<void(Octree*)> fix;
    fix = [&filePath, &fix](Octree* tree){
        if(tree->size == Octree::OctreeMax){
            tree->makeTill = Octree::OctreeMin;
            tree->file = filePath + "/" + FString::FromInt((int)tree->loc.X) + "_" 
                                        + FString::FromInt((int)tree->loc.Y) + "_" 
                                        + FString::FromInt((int)tree->loc.Z) + ".json";
        }
        else{
            for(Octree* l : tree->leafs){
                fix(l);
            }
        }
    };
    fix(root);
    
    return root;
}

Octree* Octree::makeOctree(FVector center, float octreeSize, float octreeMin, FString actorName){
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
        for(FVector side : sides){
            if(!full) break;
            full = World->OverlapBlockingTestByChannel(center+(side*distToCorner), FQuat::Identity, ECollisionChannel::ECC_WorldStatic, FCollisionShape::MakeBox(FVector(cornerSize)), params);
        }
        for(FVector corner : corners){
            if(!full) break;
            full = World->OverlapBlockingTestByChannel(center+(corner*distToCorner), FQuat::Identity, ECollisionChannel::ECC_WorldStatic, FCollisionShape::MakeBox(FVector(cornerSize)), params);
        }

        if(!full){
            // make a tree to insert
            Octree* child = new Octree(center, octreeSize);
            
            // if it still needs to be broken down, iterate through corners
            if(octreeSize > octreeMin){
                for(FVector off : corners){
                    Octree* l = makeOctree(center+(off*octreeSize/4), octreeSize/2, octreeMin, actorName);
                    if(l) child->leafs.Add(l);
                }
            }

            // if it's all the way broken down, save the normal
            else if(octreeSize == Octree::OctreeMin){
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
                // DrawDebugBox(World, center, FVector(octreeSize/2), FColor::Green, true, 2, ECC_WorldStatic, 5.0f);
            }

            return child;
        }
	}

    // DrawDebugBox(World, center, FVector(octreeSize/2), FColor::Red, true, 2, ECC_WorldStatic, 5.0f);
    return nullptr;
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
            // UE_LOG(LogHolodeck, Log, TEXT("Loading Octree %s"), *file);
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
            // UE_LOG(LogHolodeck, Log, TEXT("Making Octree %s"), *file);
            for(FVector off : corners){
                Octree* l = makeOctree(loc+(off*size/4), size/2, makeTill);
                if(l) leafs.Add(l);
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

void Octree::unload(){
    if(!isAgent && leafs.Num() != 0){
        // UE_LOG(LogHolodeck, Log, TEXT("Unloading Octree %s"), *file);
        for(Octree* leaf : leafs){
            delete leaf;
        }
        leafs.Reset();
    }
}