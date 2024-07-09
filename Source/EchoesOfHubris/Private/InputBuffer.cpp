
#include "InputBuffer.h"

void UInputBuffer::execute() {
	// Continuously execute input at the front of the queue
	return;
}

/*
Add a FInput to input buffer
Input: FInput
Output: None
*/
void UInputBuffer::addToInputBuffer(FInput newInput) {
	// If queue is already max size then skip
	if (this->inputBuffer.Num() >= INPUT_BUFFER_QUEUE_LENGTH) {
		return;
	}

	// If action already exists then reset timer to 0
	Inputs newInputAction = newInput.getInputAction();
	for (auto& input : this->inputBuffer) {
		if (input.getInputAction() == newInputAction) {
			input.setTimer(0.0);
			return;
		}
	}

	// Else add action
	this->inputBuffer.Add(newInput);

	return;
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
	return input.getTimer() >= this->INPUT_EXPIRATION_TIME;
}

/*
Returns true or false based on whether the input buffer size > 0
Input: None
Output: None
*/
bool UInputBuffer::isInputBufferEmpty() {
	return this->inputBuffer.Num() == 0;
}