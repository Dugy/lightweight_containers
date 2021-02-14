#include "scheduler.hpp"
#include <chrono>
#include <cstring>

struct CoroutineContext {
	SchedulerBase* instance = nullptr;
	int currentTask = -1;
};

CoroutineContext*& schedulerInstance() {
	static thread_local CoroutineContext* instance = nullptr;
	return instance;
};
			
struct CoroutineContextScope {
	CoroutineContext context;
	CoroutineContextScope(SchedulerBase* instance, int task = -1) : context({instance, task}) {
		schedulerInstance() = &context;
	}
	~CoroutineContextScope() {
		schedulerInstance() = nullptr;
	}
};

uint32_t now() {
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void PauserTill::await_suspend(std::coroutine_handle<>) {
	schedulerInstance()->instance->thisTaskWillWait(timeMs);
}

void Pauser::await_suspend(std::coroutine_handle<>) {
	schedulerInstance()->instance->thisTaskIsNowLowPriority();
}

PauserTill waitForMs(uint32_t timeMs) {
	return PauserTill(timeMs);
}

Pauser waitSomeTime() {
	return Pauser();
}


SchedulerBase::TaskEntry::TaskEntry() : flags(NO_FLAGS) {
}

SchedulerBase::TaskEntry::TaskEntry(Task&& task) : flags(DEFINED), task(std::move(task)), timestamp(now()) {
}

SchedulerBase::SchedulerBase(TaskEntry* entries, int entriesSize)
		: _entries(entries), _entriesSize(entriesSize) {
	memset(_entries, 0, sizeof(TaskEntry) * entriesSize);
}

bool SchedulerBase::addTask(Task&& added) {
	for (int i = 0; i < _entriesSize; i++) {
		if (_entries[i].flags ^ TaskEntry::DEFINED) {
			_entries[i].task = std::move(added);
			_entries[i].timestamp = now();
			_entries[i].flags = TaskEntry::DEFINED;
			return true;
		}
	}
	return false;
}

int SchedulerBase::taskCount() const {
	int result = 0;
	for (int i = 0; i < _entriesSize; i++) {
		if (_entries[i].flags & TaskEntry::DEFINED)
			result++;
	}
	return result;
}

constexpr int TOLERANCE = 20;
constexpr int STALE_AFTER = 3600000;

void SchedulerBase::runATask(bool alsoLowPriority) {
	uint32_t timestamp = now();
	for (int lowPriority = 0; lowPriority < 2; lowPriority++) {
		if (lowPriority && !alsoLowPriority) break;
		
		uint32_t highestPriority = 0;
		constexpr int INVALID = -1;
		int highestPriorityIndex = INVALID;
		
		for (int i = 0; i < _entriesSize; i++) {
			if (_entries[i].flags & TaskEntry::DEFINED) {
				bool condition = false;
				if ((_entries[i].flags ^ TaskEntry::LOW_PRIORITY) && !lowPriority)
					condition = _entries[i].timestamp - timestamp < TOLERANCE || timestamp - _entries[i].timestamp < STALE_AFTER;
				else if (lowPriority && (_entries[i].flags & TaskEntry::LOW_PRIORITY))
					condition = true;
				
				if (condition) {
					uint32_t priority = 0;
					if (!lowPriority)
						priority = timestamp - _entries[i].timestamp + TOLERANCE;
					else
						priority = timestamp - _entries[i].timestamp;

					if (priority > highestPriority) {
						highestPriority = priority;
						highestPriorityIndex = i;
					}
				}
			}
		}
		
		if (highestPriorityIndex != INVALID) {
			if (!lowPriority)
				_entries[highestPriorityIndex].timestamp = timestamp - STALE_AFTER - STALE_AFTER; // Will not be queued
			else
				_entries[highestPriorityIndex].timestamp = now();
				
			CoroutineContextScope keeper = {this, highestPriorityIndex};
			
			if (!_entries[highestPriorityIndex].task()) {
				_entries[highestPriorityIndex].flags = TaskEntry::NO_FLAGS;
				_entries[highestPriorityIndex].task = Task();
			}
			return;
		}
	}
}

uint32_t SchedulerBase::timeLeft() const {
	uint32_t earliest = std::numeric_limits<uint32_t>::max();
	for (int i = 0; i < _entriesSize; i++) {
		if ((_entries[i].flags & TaskEntry::DEFINED) && (_entries[i].flags ^ TaskEntry::LOW_PRIORITY)) {
			uint32_t timeLeft = _entries[i].timestamp - timeLeft;
			if (timeLeft < STALE_AFTER)
				if (timeLeft < earliest)
					earliest = timeLeft;
		}
	}
	return earliest;
}

void SchedulerBase::thisTaskWillWait(uint32_t delay) {
	int currentTask = schedulerInstance()->currentTask;
	_entries[currentTask].timestamp = now() + delay;
	_entries[currentTask].flags = decltype(TaskEntry::flags)(_entries[currentTask].flags ^ TaskEntry::LOW_PRIORITY);
}

void SchedulerBase::thisTaskIsNowLowPriority() {
	int currentTask = schedulerInstance()->currentTask;
	_entries[currentTask].timestamp = now();
	_entries[currentTask].flags = decltype(TaskEntry::flags)(_entries[currentTask].flags | TaskEntry::LOW_PRIORITY);
}
