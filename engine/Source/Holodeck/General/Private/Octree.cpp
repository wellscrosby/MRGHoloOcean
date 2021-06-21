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
FCollisionQueryParams init_params(){
            FCollisionQueryParams params;
            params.bTraceComplex = false;
            params.TraceTag = "";
            params.bFindInitialOverlaps = true;
            return params;
}
FCollisionQueryParams Octree::params = init_params();

void Octree::makeOctree(FVector center, float size, UWorld* World, TArray<Octree*>& parent, float minBox){
    /*
    * There's a bug in UE4 4.22 where if you're sweep has length less than KINDA_SMALL_NUMBER it doesn't do anything
    * https://answers.unrealengine.com/questions/887018/422-spheretraceforobjects-node-is-not-working-anym.html
    * This was fixed in 4.24, but until we update to it, we offset the end by the smallest # possible, and use bFindInitialOverlaps, bStartPenetrating to make sure we only get overlaps at the start.
    * This shouldn't effect octree generation speed if there's an overlap, but may make empty searches a smidge slower.
    */
    static FVector offset = 10*FVector(KINDA_SMALL_NUMBER, KINDA_SMALL_NUMBER, KINDA_SMALL_NUMBER) / sqrt(2.9);
    bool occup = World->SweepSingleByChannel(hit, center, center+offset, FQuat::Identity, ECollisionChannel::ECC_WorldStatic, FCollisionShape::MakeBox(FVector(size/2)), params);
    occup = hit.bStartPenetrating;

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
                    makeOctree(center+(off*size/4), size/2, World, child->leafs, minBox);
                }
            }

            // if it's all the way broken down, save the normal
            else{
                child->normal = hit.Normal;
                // DrawDebugLine(World, center, center+hit.Normal*minBox/2, FColor::Blue, true, 100, ECC_WorldStatic, 1.f);
                // DrawDebugBox(World, center, FVector(size/2), FColor::Green, true, 2, ECC_WorldStatic, .5f);
            }

            parent.Add(child);
        }
	}
    else{
        // DrawDebugBox(World, center, FVector(size/2), FColor::Red, true, 2, ECC_WorldStatic, .5f);
    }

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

void Octree::toJson(TArray<Octree*>& trees, FString filename){
    // calculate buffer size
    int64 num = 0;
    for(Octree* t : trees){
        num += t->numLeafs();
    }
    num *= 100;

    // Get values of all trees
    char* buffer = new char[num]();
    gason::JSonBuilder doc(buffer, num-1);

    doc.startObject().startArray("trees");
    for(Octree* t: trees){
        t->toJson(doc);
    }

    doc.endArray().endObject();

    if ( !doc.isBufferAdequate() ) {
        puts("warning: the buffer is small and the output json is not valid.");
    }

    // String to file
    // 
    FILE* fp = fopen(TCHAR_TO_ANSI(*filename), "w+t");
    fwrite(buffer, strlen(buffer), 1, fp);
    fclose(fp);

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

TArray<Octree*> Octree::fromJson(FString filename){
    TArray<Octree*> trees;

    // load file to a string
    std::ifstream t(*filename);
    std::string str((std::istreambuf_iterator<char>(t)),
                    std::istreambuf_iterator<char>());
    char* source = &str[0];

    char* endptr;
    gason::JsonValue json;
    gason::JsonAllocator allocator;
    int status = gason::jsonParse(source, &endptr, &json, allocator);
    if (status != gason::JSON_PARSE_OK) {
        puts("parsing failed!");
    }

    gason::JsonValue temp = json.toNode()->value;
    for(gason::JsonNode* o : temp){
        fromJson(o->value, trees);
    }

    return trees;
}

void Octree::fromJson(gason::JsonValue json, TArray<Octree*>& parent){
    Octree* child = new Octree;
    for(gason::JsonNode* o : json){
        if(o->key[0] == 'p'){
            gason::JsonNode* arr = o->value.toNode();
            child->loc = FVector(arr->value.toNumber(), arr->next->value.toNumber(), arr->next->next->value.toNumber());
        }
        if(o->key[0] == 'l'){
            for(gason::JsonNode* l : o->value){
                fromJson(l->value, child->leafs);
            }
        }
        if(o->key[0] == 'n'){
            gason::JsonNode* arr = o->value.toNode();
            child->normal = FVector(arr->value.toNumber(), arr->next->value.toNumber(), arr->next->next->value.toNumber());
        }
    }

    parent.Add(child);
}