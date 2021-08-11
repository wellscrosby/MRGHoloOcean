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
FHitResult Octree::hit = FHitResult();

FCollisionQueryParams Octree::params = Octree::init_params();

float sign(float val){
    bool s = signbit(val);
    if(s) return -1.0;
    else return 1.0;
}

bool Octree::makeOctree(FVector center, float size, UWorld* World, TArray<Octree*>& parent, float minBox, FString actorName){
    /*
    * There's a bug in UE4 4.22 where if you're sweep has length less than KINDA_SMALL_NUMBER it doesn't do anything
    * https://answers.unrealengine.com/questions/887018/422-spheretraceforobjects-node-is-not-working-anym.html
    * This was fixed in 4.24, but until we update to it, we offset the end by the smallest # possible, and use bFindInitialOverlaps, bStartPenetrating to make sure we only get overlaps at the start.
    * This shouldn't effect octree generation speed if there's an overlap, but may make empty searches a smidge slower.
    */
    static FVector offset = 10*FVector(KINDA_SMALL_NUMBER, KINDA_SMALL_NUMBER, KINDA_SMALL_NUMBER) / sqrt(2.9);
    bool occup = World->SweepSingleByChannel(hit, center, center+offset, FQuat::Identity, ECollisionChannel::ECC_WorldStatic, FCollisionShape::MakeBox(FVector(size/2)), params);
    occup = hit.bStartPenetrating;
    // if we're making for an actor, make sure we're hitting it and not something else
    if(occup && actorName != "" && actorName != hit.GetActor()->GetName()){
        occup = false;
    }

    static float cornerSize = .1;

    // if it's occupied
	if(occup){
        // Check if it's full
        bool full = true;
        // check to see if each corner is overlapping
        float distToCorner = size/2 - cornerSize;
        for(FVector corner : corners){
            full = World->OverlapBlockingTestByChannel(center+(corner*distToCorner), FQuat::Identity, ECollisionChannel::ECC_Pawn, FCollisionShape::MakeBox(FVector(cornerSize)), params);
            if(!full) break;
        }

        if(!full){
            // make a tree to insert
            Octree* child = new Octree(center);
            
            // if it still needs to be broken down, iterate through corners
            if(size > minBox){
                for(FVector off : corners){
                    makeOctree(center+(off*size/4), size/2, World, child->leafs, minBox, actorName);
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
                // DrawDebugLine(World, center, center+hit.Normal*minBox/2, FColor::Blue, true, 100, ECC_WorldStatic, 1.f);
                // DrawDebugBox(World, center, FVector(size/2), FColor::Green, true, 2, ECC_WorldStatic, .5f);
            }

            parent.Add(child);
            return true;
        }
	}

    // DrawDebugBox(World, center, FVector(size/2), FColor::Red, true, 2, ECC_WorldStatic, .5f);
    return false;

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

void Octree::toJson(FString filePath){
    // make directory
    FFileManagerGeneric().MakeDirectory(*filePath, true);

    // calculate buffer size and make writer
    int num = numLeafs()*100;
    char* buffer = new char[num]();
    gason::JSonBuilder doc(buffer, num-1);

    // fill in buffer
    toJson(doc);

    if( doc.isBufferAdequate() ){
        // String to file
        file = filePath + "/" + FString::FromInt((int)loc.X) + "_" + FString::FromInt((int)loc.Y) + "_" + FString::FromInt((int)loc.Z) + ".json";
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

    if(leafs.Num() != 0){
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
        UE_LOG(LogHolodeck, Warning, TEXT("Loading Octree %s"), *file);

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
                    load(l->value, leafs);
                }
            }
        }
    }
}

TArray<Octree*> Octree::fromFolder(FString filePath){
    TArray<Octree*> trees;
    gason::JsonAllocator allocator;

    // get files to iterate over
    TArray<FString> files;
    FFileManagerGeneric().FindFiles(files, *filePath);

    for(FString& file : files){
        // pull position from filename
        TArray<FString> out;
        file.ParseIntoArray(out, TEXT("_"), true);
        FVector position = FVector(FCString::Atof(*out[0]), FCString::Atof(*out[1]), FCString::Atof(*out[2]));

        // load file to a string
        file = filePath + "/" + file;
        
        Octree* child = new Octree(position);
        child->file = file;

        trees.Add(child);
    }

    return trees;
}

void Octree::load(gason::JsonValue& json, TArray<Octree*>& parent){
    Octree* child = new Octree;
    for(gason::JsonNode* o : json){
        if(o->key[0] == 'p'){
            gason::JsonNode* arr = o->value.toNode();
            child->loc = FVector(arr->value.toNumber(), arr->next->value.toNumber(), arr->next->next->value.toNumber());
        }
        if(o->key[0] == 'l'){
            for(gason::JsonNode* l : o->value){
                load(l->value, child->leafs);
            }
        }
        if(o->key[0] == 'n'){
            gason::JsonNode* arr = o->value.toNode();
            child->normal = FVector(arr->value.toNumber(), arr->next->value.toNumber(), arr->next->next->value.toNumber());
        }
    }

    parent.Add(child);
}
