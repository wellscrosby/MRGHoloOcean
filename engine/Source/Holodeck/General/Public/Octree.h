// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "Misc/FileHelper.h"
#include "Dom/JsonObject.h"
#include "DrawDebugHelpers.h"

// #include "Octree.generated.h"

/**
 * 
 */
// UCLASS()
// class VOXELTEST_API UOctree : public UObject
// {
// 	GENERATED_BODY()

class Octree
{
	private:
	    static FHitResult hit;
        static TArray<FVector> corners;
        static FCollisionQueryParams params;

    public:
        Octree(){};
		Octree(FVector loc) : loc(loc) {};
		~Octree(){
			for(Octree* leaf : leafs){
				delete leaf;
			}
		}

        static void makeOctree(FVector center, float size, UWorld* World, TArray<Octree*>& parent, float minBox=16);

        // functions for loading/saving octrees
        static void toJson(TArray<Octree*>& trees, FString filename);
        static TArray<Octree*> fromJson(FString filename);

        // helpers for loading/saving
        static TArray<TSharedPtr<FJsonValue>> vecToJson(FVector vec);
        TSharedPtr<FJsonObject> toJson();
        static void fromJson(TSharedPtr<FJsonObject> json, TArray<Octree*>& parent);
		
        // ignore actors
        static void ignoreActor(const AActor * InIgnoreActor){
            params.AddIgnoredActor(InIgnoreActor);
        }
        int numLeafs();

        FVector loc;
        FVector normal;
        TArray<Octree*> leafs;	
        FVector locSpherical;
        FVector normalImpact;
};
