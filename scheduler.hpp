#ifndef SCHEDULER_DUGI_HPP
#define SCHEDULER_DUGI_HPP

#include <cstdint>
#include <array>
#include <coroutine>
#include <optional>
#include <iostream>

class PauserTill {
	uint32_t timeMs;
public:
	PauserTill(uint32_t timeMs) : timeMs(timeMs) {}
	bool await_ready() {
		return false;
	}
	void await_suspend(std::coroutine_handle<> handle);
	void await_resume() {}
};

PauserTill waitForMs(uint32_t timeMs);

struct Pauser {
	bool await_ready() {
		return false;
	}
	void await_suspend(std::coroutine_handle<> handle);
	void await_resume() {}
};

Pauser waitSomeTime();

struct Task {
	struct promise_type {
		auto get_return_object() {
			return handle_type::from_promise(*this);
		}
		auto initial_suspend() {
			return std::suspend_always();
		}
		auto final_suspend() {
			return std::suspend_always();
		}
		void return_void() {
		}
		void unhandled_exception() {}
	};
	using handle_type = std::coroutine_handle<promise_type>;
	Task() { }
	Task(handle_type handle) : _handle(handle) { }
	Task& operator=(Task&& other) {
		if (_handle)
			_handle.destroy();

		_handle = std::move(other._handle);
		other._handle = handle_type();
		return *this;
	}
	Task(const Task&) = delete;
	Task(Task&& other) : _handle(std::move(other._handle)) {
		other._handle = handle_type();
	}
	bool operator()() {
		_handle.resume();
		return !_handle.done();
	}
	explicit operator bool() {
		return !_handle.done();
	}
	~Task() {
		if (_handle)
			_handle.destroy();
	}
private:
	handle_type _handle;
};

class SchedulerBase {
protected:
	struct TaskEntry {
		enum {
			NO_FLAGS = 0x0,
			DEFINED = 0x1,
			LOW_PRIORITY = 0x2
		} flags;
		uint32_t timestamp;
		Task task;
		TaskEntry();
		TaskEntry(Task&& task);
	};
	TaskEntry* _entries;
	int _entriesSize;
	
	SchedulerBase(TaskEntry* entries, int entriesSize);
	
public:
	bool addTask(Task&& added);
	int taskCount() const;
	void runATask(bool alsoLowPriority = true);
	uint32_t timeLeft() const;
	void thisTaskWillWait(uint32_t delay);
	void thisTaskIsNowLowPriority();
};

template <int size>
class Scheduler : public SchedulerBase {
	std::array<TaskEntry, size> taskSpace;
public:
	Scheduler() : SchedulerBase(taskSpace.data(), size) {}
};










#endif // SCHEDULER_DUGI_HPP
