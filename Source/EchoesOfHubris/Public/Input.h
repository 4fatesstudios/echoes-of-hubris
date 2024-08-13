
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Input.generated.h"

UENUM(BlueprintType)
enum class Inputs : uint8 {
	NONE,
	ATTACK,
	JUMP,
	JUMPBURST,
	DASHBURST
};


USTRUCT(BlueprintType)
struct ECHOESOFHUBRIS_API FInput {
	GENERATED_BODY()

private:
	float timer;
	Inputs inputAction;
	bool isSuccessful;

public:
	FInput() {
		this->timer = 0.0;
		this->inputAction = Inputs::NONE;
		this->isSuccessful = false;
	}

	FInput(Inputs action, float time) {
		this->timer = time;
		this->inputAction = action;
	}

	FInput(Inputs action, float time, bool isSuccess) {
		this->timer = time;
		this->inputAction = action;
		this->isSuccessful = isSuccess;
	}

	float getTimer() const { return this->timer; }
	void setTimer(float time) { this->timer = time; }

	Inputs getInputAction() const { return this->inputAction; }
	void setInputAction(Inputs input) { this->inputAction = input; }

	bool getIsSuccessful() const { return this->isSuccessful; }
	void setIsSuccessful(bool isSuccess) { this->isSuccessful = isSuccess; }

	// Overload the == operator
	bool operator==(const FInput& Other) const {
		return this->inputAction == Other.inputAction;
	}

};