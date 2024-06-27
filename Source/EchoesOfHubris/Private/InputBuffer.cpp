// Fill out your copyright notice in the Description page of Project Settings.


#include "InputBuffer.h"

bool UInputBuffer::isInputBufferEmpty() {
	//return this->inputBuffer.size() > 0;
	return true;
}

std::vector<Input> UInputBuffer::getInputBuffer() {
	return this->inputBuffer;
}

void UInputBuffer::addToInputBuffer(Input newInput) {
	this->inputBuffer.push_back(newInput);
}

Input UInputBuffer::createNewInput(Inputs input) {
	Input i = Input();
	i.inputAction = input;
	i.timer = 0.0;
	i.isSuccessful = false;
	return i;
}

void UInputBuffer::increaseTimer(Input &input) {
	input.timer += 0.1;
}

bool UInputBuffer::isExpired(Input input) {
	return input.timer == this->INPUT_EXPIRATION_TIME;
}

