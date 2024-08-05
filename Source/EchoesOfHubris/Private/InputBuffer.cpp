
#include "InputBuffer.h"

TArray<FInput> UInputBuffer::inputBuffer;
const AActor* UInputBuffer::ReferenceActor = nullptr;

void UInputBuffer::execute() {
	// Simulate action functions need to set FInput isSuccessful to true

	for (auto& currInput : inputBuffer) {
		while (!isExpired(currInput)) {
			// Call input simulate function
			Inputs inputToExecute = currInput.getInputAction();
			switch (inputToExecute) {
			case Inputs::NONE:
				// Do nothing
				break;
			case Inputs::ATTACK:
				// Call simulate ATTACK
				if (currInput.getIsSuccessful()) {
					inputSuccess(inputToExecute); // Remove input if success
				}
				else {
					increaseTimer(currInput);
				}
				break;
			case Inputs::JUMP:
				// Call simulate JUMP
				if (currInput.getIsSuccessful()) {
					inputSuccess(inputToExecute); // Remove input if success
				}
				else {
					increaseTimer(currInput);
				}
				break;
			case Inputs::JUMPBURST:
				// Call simulate JUMPBURST
				if (currInput.getIsSuccessful()) {
					inputSuccess(inputToExecute); // Remove input if success
				}
				else {
					increaseTimer(currInput);
				}
				break;
			case Inputs::DASHBURST:
				// Call simulate DASHBURST
				if (currInput.getIsSuccessful()) {
					inputSuccess(inputToExecute); // Remove input if success
				}
				else {
					increaseTimer(currInput);
				}
				break;
			}
			// Clear expired input at the top
			if (isExpired(currInput)) {
				inputBuffer.RemoveAt(0);
			}
		}
	}
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
	FInput input{ FInput(newInput, INPUT_EXPIRATION_TIME, false) };
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
			input.setIsSuccessful(true);
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