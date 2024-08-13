
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
	static TArray<FInput> inputBuffer;
	static constexpr float INPUT_EXPIRATION_TIME = 0.5f;
	static constexpr unsigned int INPUT_BUFFER_QUEUE_LENGTH = 5;
	static const AActor* ReferenceActor;

public:
	TArray<FInput> GetInputBuffer() const { return this->inputBuffer; }

	UFUNCTION(BlueprintCallable, Category = "Input Buffer")
	static void addToInputBuffer(Inputs newInput);

	UFUNCTION(BlueprintCallable, Category = "Input Buffer")
	static void inputSuccess(Inputs newInput);

	static void increaseTimer(FInput& input);
	static bool isExpired(FInput input);
	static bool isInputBufferEmpty();
	
	static void execute();
	static void increaseTimerAndCheckIfExpired();

	UFUNCTION(BlueprintCallable, Category = "Input Buffer")
	static void setActorRef(AActor* actorRef);
};