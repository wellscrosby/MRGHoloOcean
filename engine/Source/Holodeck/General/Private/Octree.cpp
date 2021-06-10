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

    static int cornerSize = .1;

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
        int num = 0;
        for(Octree* leaf : leafs){
            num += leaf->numLeafs();
        }
        return num;
    }
}

void Octree::toJson(TArray<Octree*>& trees, FString filename){
    // Get values of all trees
    TArray<TSharedPtr<FJsonValue>> treesJson;
    for(Octree* tree : trees){
        // have to convert to different type
        TSharedRef<FJsonValueObject> treeJson = MakeShareable(new FJsonValueObject( tree->toJson() ));
        treesJson.Add(treeJson);
    }

    // Make Json object and put them in
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
    JsonObject->SetArrayField("trees", treesJson);

    // Output to string
    FString asString;
    TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&asString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

    // String to file
    FFileHelper::SaveStringToFile(asString, *filename);
}

TSharedPtr<FJsonObject> Octree::toJson(){
    // create and fill in json for this tree
    TSharedPtr<FJsonObject> json = MakeShareable(new FJsonObject);
    json->SetArrayField("p", vecToJson(loc));
    if(leafs.Num()==0){
        json->SetArrayField("n", vecToJson(normal));
    }
    else{
        TArray<TSharedPtr<FJsonValue>> leafsJson;
        for(Octree* leaf : leafs){
            // have to convert to different type
            TSharedRef<FJsonValueObject> leafJson = MakeShareable(new FJsonValueObject( leaf->toJson() ));
            leafsJson.Add(leafJson);
        }
        json->SetArrayField("l", leafsJson);
    }

    return json;
}

TArray<TSharedPtr<FJsonValue>> Octree::vecToJson(FVector vec){
    TArray<TSharedPtr<FJsonValue>> json;
    for(int i=0; i<3; i++){
        TSharedPtr<FJsonValue> item = MakeShareable(new FJsonValueNumber(vec.Component(i)));
        json.Add(item);
    }
    return json;
}

TArray<Octree*> Octree::fromJson(FString filename){
    TArray<Octree*> trees;

    // load file to a string
    FString jsonString;
    FFileHelper::LoadFileToString(jsonString,*filename);

    // convert string to json, and get moving
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
	TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(jsonString);
 
    if (FJsonSerializer::Deserialize(JsonReader, JsonObject) && JsonObject.IsValid())
	{
        //The person "object" that is retrieved from the given json file
        TArray<TSharedPtr<FJsonValue>> treesJson = JsonObject->GetArrayField("trees");
    
        for(auto tJson : treesJson){
            fromJson(tJson->AsObject(), trees);
        }
    }
    else{
        UE_LOG(LogTemp, Warning, TEXT("Couldn't deserialize %s"), *filename);
    }

    return trees;
}

void Octree::fromJson(TSharedPtr<FJsonObject> json, TArray<Octree*>& parent){
    // put in location
    TArray<TSharedPtr<FJsonValue>> locJson = json->GetArrayField("p");
    FVector loc = FVector(locJson[0]->AsNumber(), locJson[1]->AsNumber(), locJson[2]->AsNumber());
    Octree * child = new Octree(loc);

    // if it has children put them in
    if(json->HasField("l")){
        for(auto l : json->GetArrayField("l")){
            fromJson(l->AsObject(), child->leafs);
        }
    }
    // otherwise it's a leaf, put in normal
    else{
        TArray<TSharedPtr<FJsonValue>> normalJson = json->GetArrayField("n");
        child->normal = FVector(normalJson[0]->AsNumber(), normalJson[1]->AsNumber(), normalJson[2]->AsNumber());
    }

    parent.Add(child);
}