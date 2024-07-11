
#include "InputBuffer.h"

TArray<FInput> UInputBuffer::inputBuffer;
const AActor* UInputBuffer::ReferenceActor = nullptr;

void UInputBuffer::execute() {
	//ReferenceActor.Attack()
	return;
}

/*
Add a FInput to input buffer
Input: Inputs
Output: None
*/
void UInputBuffer::addToInputBuffer(Inputs newInput) {
	// If queue is already max size then skip
	if (inputBuffer.Num() >= INPUT_BUFFER_QUEUE_LENGTH) {
		return;
	}

	// If action already exists then reset timer to 0
	for (auto& input : inputBuffer) {
		if (input.getInputAction() == newInput) {
			input.setTimer(0.0);
			return;
		}
	}

	// Else add action
	FInput input{ FInput(newInput, INPUT_EXPIRATION_TIME) };
	inputBuffer.Add(input);

	return;
}

/*
Successful call on Input
Input: Inputs
Output: None
*/
void UInputBuffer::inputSuccess(Inputs newInput) {
	for (auto& input : inputBuffer) {
		if (input.getInputAction() == newInput) {
			inputBuffer.Remove(input);
			return;
		}
	}
}

/*
Increase given input's timer by 0.1
Input: FInput
Output: None
*/
void UInputBuffer::increaseTimer(FInput &input) {
	// Increase input timer by 0.1
	float currTimer = input.getTimer();
	input.setTimer(currTimer + 0.1);
	return;
}

/*
Check whether given input's timer is greater than or equal to INPUT_EXPIRATION_TIME
INPUT_EXPIRATION_TIME = 0.5
Input: FInput
Output: None
*/
bool UInputBuffer::isExpired(FInput input) {
	return input.getTimer() >= INPUT_EXPIRATION_TIME;
}

/*
Returns true or false based on whether the input buffer size > 0
Input: None
Output: None
*/
bool UInputBuffer::isInputBufferEmpty() {
	return inputBuffer.Num() == 0;
}

/*
* 
*/
void UInputBuffer::setActorRef(AActor* actorRef) {
	ReferenceActor = actorRef;
}