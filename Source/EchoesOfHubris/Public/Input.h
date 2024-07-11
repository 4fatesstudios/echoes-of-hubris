
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Input.generated.h"

UENUM(BlueprintType)
enum class Inputs : uint8 {
	NONE,
	JUMP,
	MOVE_LEFT,
	MOVE_RIGHT
};


USTRUCT(BlueprintType)
struct ECHOESOFHUBRIS_API FInput {
	GENERATED_BODY()

private:
	float timer;
	Inputs inputAction;

public:
	FInput() {
		this->timer = 0.0;
		this->inputAction = Inputs::NONE;
	}

	FInput(Inputs action, float time) {
		this->timer = time;
		this->inputAction = action;
	}

	float getTimer() const { return this->timer; }
	void setTimer(float time) { this->timer = time; }

	Inputs getInputAction() const { return this->inputAction; }
	void setInputAction(Inputs input) { this->inputAction = input; }

	// Overload the == operator
	bool operator==(const FInput& Other) const {
		return this->inputAction == Other.inputAction;
	}

};