#pragma once

class Timer {
public:
	Timer();
	void Tick();

	double TPS;      // Ticks per second
	double interval; // The time passed since the last call of Tick()
};
