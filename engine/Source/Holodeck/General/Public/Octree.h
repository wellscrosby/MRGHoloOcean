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
#include <functional>

class Octree
{
	private:
        // Globals used for calculations
        static TArray<FVector> corners;
        static TArray<FVector> sides;
        static FCollisionQueryParams params;
        static FVector offset;
        static float cornerSize;
        static FVector EnvMin;
        static FVector EnvMax;
        static UWorld* World;

        static FVector EnvCenter;

        static void loadJson(gason::JsonValue& json, TArray<Octree*>& parent, float size);
        void toJson(gason::JSonBuilder& doc);

        static FCollisionQueryParams init_params(){
            FCollisionQueryParams p;
            p.bTraceComplex = false;
            p.TraceTag = "";
            p.bFindInitialOverlaps = true;
            return p;
        }

        float sizeLeaf;

    public:
        static float OctreeRoot;
        static float OctreeMax;
        static float OctreeMin;

        Octree(){};
		Octree(FVector loc, float size, float sizeLeaf, FString file="") : sizeLeaf(sizeLeaf), size(size), loc(loc), file(file) {};
		~Octree(){ 
            for(Octree* leaf : leafs){
                delete leaf;
            }
            leafs.Reset();
        }

        // Used to setup octree globals
        static void initOctree();

        // Figures out where octree roots are
        static Octree* makeEnvOctreeRoot(UWorld* w);

        // iterative constructs octree
        static Octree* makeOctree(FVector center, float octreeSize, float octreeMin, bool recurse, FString actorName="");

        void unload();
        void load();

        // helpers for saving
        void toJson();
		
        // ignore actors
        static void ignoreActor(const AActor * InIgnoreActor){
            params.AddIgnoredActor(InIgnoreActor);
        }
        static void resetParams(){ params = init_params(); }

        int numLeafs();

        // Used to check if it's a dynamic octree for an agent
        bool isAgent;
        
        // Given to all
        float size;
        FVector loc;

        // Given to octree roots that have been saved/loaded from file
        FString file;

        // Given to each non-leaf
        TArray<Octree*> leafs;

        // Given to each leaf 
        FVector normal;

        // Used during computations
        FVector locSpherical;
        FIntVector idx;
        float val;
};
