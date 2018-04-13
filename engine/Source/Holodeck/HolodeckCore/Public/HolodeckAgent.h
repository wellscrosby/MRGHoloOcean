// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Holodeck.h"

#include "GameFramework/Pawn.h"
#include "HolodeckPawnController.h"

#include "HolodeckAgent.generated.h"

/**
* AHolodeckAgent
* The base class for holodeck agents. 
* HolodeckAgents are what are controllable from the python side of the code. HolodeckAgents will 
* tell their controller to open up the channels needed for giving commands across the python binding.
*/
UCLASS()
class AHolodeckAgent : public APawn
{
	GENERATED_BODY()

public:
	/**
	  * Default Constructor
	  */
	AHolodeckAgent();

	/**
	  * BeginPlay
	  * Called when the game starts.
	  * Registers the reward and terminal signals.
	  */
	void BeginPlay() override;

	/**
	  * Tick
	  * Ticks the agent.
	  * If it is overridden, it must be called by the child class!
	  * @param DeltaSeconds the time since the last tick.
	  */
	void Tick(float DeltaSeconds) override;

	// Must be set in the editor.
	UPROPERTY(EditAnywhere)
	FString AgentName;

	/**
	  * SetReward
	  * Sets the reward in the server for this agent.
	  * @param Reward the value of the reward.
	  */
	void SetReward(int Reward) { if (RewardPtr != nullptr) *RewardPtr = Reward; };

	/**
	  * SetTerminal
	  * Sets the terminal in the server for this agent.
	  * @param Terminal the value of the terminal signal.
	  */
	void SetTerminal(bool Terminal) { if (TerminalPtr != nullptr) *TerminalPtr = Terminal; };

	/**
	* Teleport
	* Instantly moves the agent to target location, with the orientation that was given
	* If no orientation was given, orientation remains unchanged (see overloaded function)
	* @param NewLocation The location to move to
	* @param NewRotation The rotation that the object will take on
	* @return Bool if the teleport was successful.
	*/
	bool Teleport(const FVector& NewLocation, FRotator NewRotation);

	/**
	* Teleport
	* Instantly moves the agent to target location, with the orientation that was given
	* Orientation remains unchanged
	* @param NewLocation The location to move to
	* @return Bool if the teleport was successful.
	*/
	bool Teleport(const FVector& NewLocation);

	/**
	* InitializeController
	* Hooks up everything with the controller. This is normally called in the beginPlay function, 
	* but if you have to manually configure a controller, you will have to call this function after
	* you do it. 
	*/
	bool InitializeController();

private:
	float* RewardPtr;
	bool* TerminalPtr;
	AHolodeckPawnController* HolodeckController;
};
