#include "scheduler.hpp"
#include <iostream>
#include <chrono>
#include <thread>

template <typename T>
using StaticAwaitable = Awaitable<T, StaticAllocator<>>;

StaticAwaitable<int> workForLong() {
	std::cout << "Working for long" << std::endl;
	co_await waitForMs(100);
	co_return 3;
}

StaticAwaitable<int> work() {
	std::cout << "Working" << std::endl;
	co_return co_await workForLong();
}

Task messAround() {
	while (true) {
		std::cout << "Messing around" << std::endl;
		co_await waitSomeTime();
	}
}

Task chillOut() {
	while (true) {
		std::cout << "Chilling out" << std::endl;
		co_await waitForMs(150);
	}
}

Task slack() {
	while (true) {
		std::cout << "Slacking" << std::endl;
		int worked = co_await work();
		std::cout << "Slacking after work with result " << worked << std::endl;
		co_await waitForMs(250);
	}
}

int main() {
	Scheduler<16> scheduler;
	scheduler.addTask(messAround());
	scheduler.addTask(chillOut());
	scheduler.addTask(slack());
	for (int i = 0; i < 30; i++) {
		scheduler.runATask();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}
