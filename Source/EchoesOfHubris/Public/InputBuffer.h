// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include <vector>
#include <string>
#include "InputBuffer.generated.h"

enum Inputs { JUMP };

struct Input {
	Inputs inputAction;
	float timer;
	bool isSuccessful;
};

/**
 * 
 */
UCLASS()
class ECHOESOFHUBRIS_API UInputBuffer : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
private:
	const float INPUT_EXPIRATION_TIME = 0.5;
	const unsigned int INPUT_BUFFER_QUEUE_LENGTH = 5;
	std::vector<Input> inputBuffer;

public:
	void addToInputBuffer(Input newInput);
	Input createNewInput(Inputs input);
	void increaseTimer(Input& input);
	bool isExpired(Input input);
	bool isInputBufferEmpty();

	std::vector<Input> getInputBuffer();
	
};