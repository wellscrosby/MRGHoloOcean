// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManagerGeneric.h"
#include "DrawDebugHelpers.h"

#include "gason.h"
#include "jsonbuilder.h"
#include <string>
#include <fstream>
#include <streambuf>

class Octree
{
	private:
        // Globals used for calculations
        static TArray<FVector> corners;
        static FCollisionQueryParams params;
        static FVector offset;
        static float cornerSize;

    public:
        static float OctreeMax;
        static float OctreeMin;
        static UWorld* World;

        Octree(){};
		Octree(FVector loc, float size) : size(size), loc(loc) {};
		Octree(FVector loc, float size, FString file) : size(size), loc(loc), file(file) {};
		~Octree(){ 
            for(Octree* leaf : leafs){
                delete leaf;
            }
            leafs.Reset();
        }

        static Octree* newHeadOctree(FVector center, float octreeSize, FString filePath, FString actorName="");
        static void makeOctree(FVector center, float octreeSize, TArray<Octree*>& parent, FString actorName="");

        void unload(){
            if(leafs.Num() != 0){
                UE_LOG(LogHolodeck, Warning, TEXT("Unloading Octree %s"), *file);
                for(Octree* leaf : leafs){
                    delete leaf;
                }
                leafs.Reset();
            }
        }
        void load();
        static void load(gason::JsonValue& json, TArray<Octree*>& parent);

        // helpers for saving
        void toJson();
        void toJson(gason::JSonBuilder& doc);
		
        // ignore actors
        static void ignoreActor(const AActor * InIgnoreActor){
            params.AddIgnoredActor(InIgnoreActor);
        }
        static void resetParams(){ params = init_params(); }
        int numLeafs();

        static FCollisionQueryParams init_params(){
            FCollisionQueryParams p;
            p.bTraceComplex = false;
            p.TraceTag = "";
            p.bFindInitialOverlaps = true;
            return p;
        }

        // Given to all
        float size;
        FVector loc;

        // Given to octree's that have been saved/loaded from file
        FString file;

        // Given to each non-leaf
        TArray<Octree*> leafs;

        // Given to each leaf 
        FVector normal;

        // Used during computations
        FVector locSpherical;
        float val;
};
