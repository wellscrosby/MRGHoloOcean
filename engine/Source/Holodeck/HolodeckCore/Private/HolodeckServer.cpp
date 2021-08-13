// Created by joshgreaves on 5/9/17.

#include "Holodeck.h"
#include "HolodeckServer.h"

UHolodeckServer::UHolodeckServer() {
    // Warning -- This class gets initialized a few times by Unreal because it is a UObject.
    // DO NOT rely on singleton-qualities in the constructor, only in the Start() function
    bIsRunning = false;
}

UHolodeckServer::~UHolodeckServer() {
    Kill();

    // if(octree.Num() != 0){
	// 	for(Octree* t : octree){
	// 		delete t;
	// 	}
	// 	octree.Empty();
	// }
}

void UHolodeckServer::Start() {
    UE_LOG(LogHolodeck, Log, TEXT("Initializing HolodeckServer"));
    if (bIsRunning) {
        UE_LOG(LogHolodeck, Warning, TEXT("HolodeckServer is already running! Bringing it down and up"));
        Kill();
    }

    if (!FParse::Value(FCommandLine::Get(), TEXT("HolodeckUUID="), UUID))
        UUID = "";
    UE_LOG(LogHolodeck, Log, TEXT("UUID: %s"), *UUID);

    // Load environment size
    if (!FParse::Value(FCommandLine::Get(), TEXT("EnvMinX="), EnvMin.X)) EnvMin.X = -10;
    if (!FParse::Value(FCommandLine::Get(), TEXT("EnvMinY="), EnvMin.Y)) EnvMin.Y = -10;
    if (!FParse::Value(FCommandLine::Get(), TEXT("EnvMinZ="), EnvMin.Z)) EnvMin.Z = -10;
    if (!FParse::Value(FCommandLine::Get(), TEXT("EnvMaxX="), EnvMax.X)) EnvMax.X = 10;
    if (!FParse::Value(FCommandLine::Get(), TEXT("EnvMaxY="), EnvMax.Y)) EnvMax.Y = 10;
    if (!FParse::Value(FCommandLine::Get(), TEXT("EnvMaxZ="), EnvMax.Z)) EnvMax.Z = 10;
    // Clean environment size
    FVector min = FVector((int)FGenericPlatformMath::Min(EnvMin.X, EnvMax.X), (int)FGenericPlatformMath::Min(-1*EnvMin.Y, -1*EnvMax.Y), (int)FGenericPlatformMath::Min(EnvMin.Z, EnvMax.Z));
    FVector max = FVector((int)FGenericPlatformMath::Max(EnvMin.X, EnvMax.X), (int)FGenericPlatformMath::Max(-1*EnvMin.Y, -1*EnvMax.Y), (int)FGenericPlatformMath::Max(EnvMin.Z, EnvMax.Z));
    EnvMin = min*100;
    EnvMax = max*100;
    UE_LOG(LogHolodeck, Log, TEXT("EnvMin: %s"), *EnvMin.ToString());
    UE_LOG(LogHolodeck, Log, TEXT("EnvMax: %s"), *EnvMax.ToString());

    // Get octree min/max
    float tempVal;
    if (!FParse::Value(FCommandLine::Get(), TEXT("OctreeMin="), tempVal)) tempVal = .1;
    OctreeMin = (int)(tempVal*100);
    if (!FParse::Value(FCommandLine::Get(), TEXT("OctreeMax="), tempVal)) tempVal = 5;
    OctreeMax = (int)(tempVal*100);

    // Make max a multiple of min
    tempVal = OctreeMin;
    while(tempVal <= OctreeMax){
        tempVal *= 2;
    }
    OctreeMax = tempVal;
    UE_LOG(LogHolodeck, Log, TEXT("OctreeMin: %d, OctreeMax: %d"), OctreeMin, OctreeMax);

    // Pass those values to Octree
    Octree::initOctree(OctreeMin, OctreeMax, EnvMin, EnvMax);

#if PLATFORM_WINDOWS
    auto LoadingSemaphore = OpenSemaphore(EVENT_ALL_ACCESS, false, *(LOADING_SEMAPHORE_PATH + UUID));
    ReleaseSemaphore(LoadingSemaphore, 1, NULL);
    this->LockingSemaphore1 = CreateSemaphore(NULL, 1, 1, *(SEMAPHORE_PATH1 + UUID));
    this->LockingSemaphore2 = CreateSemaphore(NULL, 0, 1, *(SEMAPHORE_PATH2 + UUID));
#elif PLATFORM_LINUX
    auto LoadingSemaphore = sem_open(TCHAR_TO_ANSI(*(LOADING_SEMAPHORE_PATH + UUID)), O_CREAT, 0777, 0);
    if (LoadingSemaphore == SEM_FAILED) {
        LogSystemError("Unable to open loading semaphore");
    }

    LockingSemaphore1 = sem_open(TCHAR_TO_ANSI(*(SEMAPHORE_PATH1 + UUID)), O_CREAT, 0777, 1);
    if (LockingSemaphore1 == SEM_FAILED) {
        LogSystemError("Unable to open server semaphore");
    }

    LockingSemaphore2 = sem_open(TCHAR_TO_ANSI(*(SEMAPHORE_PATH2 + UUID)), O_CREAT, 0777, 0);
    if (LockingSemaphore2 == SEM_FAILED) {
        LogSystemError("Unable to open client semaphore");
    }

    int status = sem_post(LoadingSemaphore);
    if (status == -1) {
        LogSystemError("Unable to update loading semaphore");
    }

    // Client unlinks LoadingSemaphore
#endif

    bIsRunning = true;
    UE_LOG(LogHolodeck, Log, TEXT("HolodeckServer started successfully"));
}

void UHolodeckServer::Kill() {
    UE_LOG(LogHolodeck, Log, TEXT("Killing HolodeckServer"));
    if (!bIsRunning) return;

    Memory.clear();

#if PLATFORM_WINDOWS
    CloseHandle(this->LockingSemaphore1);
    CloseHandle(this->LockingSemaphore2);
#elif PLATFORM_LINUX
    int status = sem_unlink(SEMAPHORE_PATH1);
    if (status == -1) {
        LogSystemError("Unable to close server semaphore");
    }

    status = sem_unlink(SEMAPHORE_PATH2);
    if (status == -1) {
        LogSystemError("Unable to close client semaphore");
    }
#endif

    bIsRunning = false;
    UE_LOG(LogHolodeck, Log, TEXT("HolodeckServer successfully shut down"));
}

void* UHolodeckServer::Malloc(const std::string& Key, unsigned int BufferSize) {
    // If this key doesn't already exist, or the buffer size has changed, allocate the memory.
    if (!Memory.count(Key) || Memory[Key]->Size() != BufferSize) {
        UE_LOG(LogHolodeck, Log, TEXT("Mallocing %u bytes for key %s"), BufferSize, UTF8_TO_TCHAR(Key.c_str()));
        Memory[Key] = std::unique_ptr<HolodeckSharedMemory>(new HolodeckSharedMemory(Key, BufferSize, TCHAR_TO_UTF8(*UUID)));
    }
    return Memory[Key]->GetPtr();
}

void UHolodeckServer::Acquire() {
    UE_LOG(LogHolodeck, VeryVerbose, TEXT("HolodeckServer Acquiring"));
#if PLATFORM_WINDOWS
    WaitForSingleObject(this->LockingSemaphore1, INFINITE);
#elif PLATFORM_LINUX

    int status = sem_wait(LockingSemaphore1);
    if (status == -1) {
        LogSystemError("Unable to wait for server semaphore");
    }
#endif
}

void UHolodeckServer::Release() {
    UE_LOG(LogHolodeck, VeryVerbose, TEXT("HolodeckServer Releasing"));
#if PLATFORM_WINDOWS
    ReleaseSemaphore(this->LockingSemaphore2, 1, NULL);
#elif PLATFORM_LINUX

    int status = sem_post(LockingSemaphore2);
    if (status == -1) {
        LogSystemError("Unable to update loading semaphore");
    }
#endif
}

bool UHolodeckServer::IsRunning() const {
    return bIsRunning;
}

void UHolodeckServer::LogSystemError(const std::string& errorMessage) {
    UE_LOG(LogHolodeck, Fatal, TEXT("%s - Error code: %d=%s"), ANSI_TO_TCHAR(errorMessage.c_str()), errno, ANSI_TO_TCHAR(strerror(errno)));
}

void UHolodeckServer::makeOctree(UWorld* World){
    if(octree.Num() == 0){
        Octree::OctreeMin = OctreeMin;
        Octree::OctreeMax = OctreeMax;
        Octree::World = World;

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
                center = FVector(l[0], l[1], l[2]);
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

            // Save these!
            FString root = "";
            for(Octree* o : octree){
                root += FString::FromInt((int)o->loc.X)
                         + " " + FString::FromInt((int)o->loc.Y)
                         + " " + FString::FromInt((int)o->loc.Z)
                         + "\n";
            }
            FFileHelper::SaveStringToFile(root, *rootFile);
        }

    }
}