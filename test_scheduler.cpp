#include "scheduler.hpp"
#include <iostream>
#include <chrono>
#include <thread>

Task messAround() {
	while (true) {
		std::cout << "Messing around" << std::endl;
		co_await waitForMs(250);
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
		co_await waitSomeTime();
	}
}

Task work() {
	std::cout << "Working" << std::endl;
	co_return;
}

int main() {
	Scheduler<16> scheduler;
	scheduler.addTask(messAround());
	scheduler.addTask(chillOut());
	scheduler.addTask(slack());
	scheduler.addTask(work());
	for (int i = 0; i < 20; i++) {
		scheduler.runATask();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}
