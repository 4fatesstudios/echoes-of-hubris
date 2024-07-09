
#pragma once

#include "Input.h"
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "InputBuffer.generated.h"


/**
 * 
 */
UCLASS()
class ECHOESOFHUBRIS_API UInputBuffer : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
private:
	TArray<FInput> inputBuffer;
	static constexpr float INPUT_EXPIRATION_TIME = 0.5f;
	static constexpr unsigned int INPUT_BUFFER_QUEUE_LENGTH = 5;

public:
	TArray<FInput> GetInputBuffer() const { return this->inputBuffer; }

	void addToInputBuffer(FInput newInput);
	void increaseTimer(FInput& input);
	bool isExpired(FInput input);
	bool isInputBufferEmpty();
	
	void execute();
};