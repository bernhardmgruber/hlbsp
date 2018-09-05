#include "Timer.h"

#include <chrono>

namespace {
	constexpr auto TPS_UPDATE_INTERVAL = 0.2;
}

Timer::Timer() {
	Tick();
}

void Timer::Tick() {
	using time_point = decltype(std::chrono::high_resolution_clock::now());
	static time_point lastTime;

	auto now = std::chrono::high_resolution_clock::now();
	interval = std::chrono::duration<double>(now - lastTime).count();

	lastTime = now;

	static std::size_t nFrameCounter = 0;
	static time_point dLastTimeFPSUpdate = {};

	nFrameCounter++;

	const auto timeSinceLastFPSUpdate = std::chrono::duration<double>(now - dLastTimeFPSUpdate).count();
	if (timeSinceLastFPSUpdate > TPS_UPDATE_INTERVAL) {
		TPS = nFrameCounter / timeSinceLastFPSUpdate;
		dLastTimeFPSUpdate = now;
		nFrameCounter = 0;
	}
}
