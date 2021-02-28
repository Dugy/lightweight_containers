#ifndef SCHEDULER_DUGI_HPP
#define SCHEDULER_DUGI_HPP

#include <cstdint>
#include <array>
#include <coroutine>
#include <optional>
#include <iostream>
#include <memory>

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

struct SchedulerBase;

struct TaskBase {
	static SchedulerBase* getScheduler();
};

struct Task : TaskBase {
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

template <typename T, typename Allocator = std::allocator<void*>>
struct Awaitable : private TaskBase {
	struct promise_type {
		T returned;
		auto get_return_object() {
			return handle_type::from_promise(*this);
		}
		auto initial_suspend() {
			return std::suspend_always();
		}
		auto final_suspend() {
			return std::suspend_always();
		}
		void return_value(const T& value) {
			returned = value;
		}
		void unhandled_exception() {}
		
		void* operator new(size_t size)
		{
			Allocator allocator;
			return allocator.allocate(size);
		}

		void operator delete(void* p)
		{
			Allocator allocator;
			allocator.deallocate(reinterpret_cast<void**>(p), sizeof(promise_type));
		}
	
		~promise_type() {
		}
	};
	using handle_type = std::coroutine_handle<promise_type>;
	Awaitable() { }
	Awaitable(handle_type handle) : _handle(handle) { }
	Awaitable& operator=(Task&& other) {
		if (_handle)
			_handle.destroy();

		_handle = std::move(other._handle);
		other._handle = handle_type();
		return *this;
	}
	Awaitable(const Awaitable&) = delete;
	Awaitable(Awaitable&& other) : _handle(std::move(other._handle)) {
		other._handle = handle_type();
	}
	bool operator()() {
		_handle.resume();
		return !_handle.done();
	}
	explicit operator bool() {
		return !_handle.done();
	}
	~Awaitable() {
		if (_handle)
			_handle.destroy();
	}
	
	bool await_ready();
	void await_suspend(std::coroutine_handle<> handle) {
	}
	T await_resume() {
		return _handle.promise().returned;
	}
private:
	handle_type _handle;
	friend class SchedulerBase;
};

class SchedulerBase {
protected:
	struct TaskEntry {
		constexpr static int NOT_DEPENDED = -1;
		enum : uint8_t {
			NO_FLAGS = 0x0,
			DEFINED = 0x1,
			LOW_PRIORITY = 0x2,
			AWAITING = 0x4,
		} flags;
		int16_t depended;
		uint32_t timestamp;
		alignas(void*) std::array<bool, std::max(sizeof(Task), sizeof(Awaitable<int>))> memory;
		void (*run)(TaskEntry* self, SchedulerBase* scheduler, bool justDestroy);
		TaskEntry();
		TaskEntry(Task&& task);
		template <typename T>
		TaskEntry(Awaitable<T>* awaitable);
	};
	TaskEntry* _entries;
	int _entriesSize;
	
	SchedulerBase(TaskEntry* entries, int entriesSize);
	~SchedulerBase();
	
	TaskEntry* addTaskHelper();
	static int currentCoroutine();
	
public:
	int taskCount() const;
	void runATask(bool alsoLowPriority = true);
	uint32_t timeLeft() const;
	void thisTaskWillWait(uint32_t delay);
	void thisTaskIsNowLowPriority();
	
	bool addTask(Task&& added);
	template <typename T, typename Allocator>
	bool addTask(Awaitable<T, Allocator>* added) {
		TaskEntry* place = addTaskHelper();
		if (!place)
			return false;
		*reinterpret_cast<Awaitable<T, Allocator>**>(&place->memory) = added;
		place->depended = currentCoroutine();
		_entries[place->depended].timestamp = place->timestamp;
		place->run = [] (TaskEntry* self, SchedulerBase* scheduler, bool justDestroy) {
			Awaitable<T, Allocator>*& awaitable = reinterpret_cast<Awaitable<T, Allocator>*&>(self->memory);
			if (justDestroy || !awaitable->operator()()) {
				self->flags = TaskEntry::NO_FLAGS;
				awaitable = nullptr;
				if (self->depended != TaskEntry::NOT_DEPENDED) {
					scheduler->_entries[self->depended].flags = decltype(TaskEntry::flags)(scheduler->_entries[self->depended].flags ^ TaskEntry::AWAITING);
					self->depended = TaskEntry::NOT_DEPENDED;
				}
			}
		};
		_entries[place->depended].flags = decltype(TaskEntry::flags)(_entries[place->depended].flags | TaskEntry::AWAITING);
		return true;
	};
};

template <int size>
class Scheduler : public SchedulerBase {
	std::array<TaskEntry, size> taskSpace;
public:
	Scheduler() : SchedulerBase(taskSpace.data(), size) {}
};

template <typename T, typename Allocator>
bool Awaitable<T, Allocator>::await_ready() {
	getScheduler()->addTask(this);
	return false;
}

template <int ElementCount = 16, int Size = 80>
struct StaticAllocator {
	class StaticAllocatingSpace {
		alignas(void*) std::array<int8_t, ElementCount * Size> buffer;
		std::array<bool, ElementCount> usage;
		static_assert(Size % sizeof(void*) == 0, "ElementSize must be divisible by pointer size");
		friend struct StaticAllocator;
	};
	static inline StaticAllocatingSpace _space;

	StaticAllocator() = default;
	template <int OtherElementCount, int OtherSize> constexpr StaticAllocator(const StaticAllocator<OtherElementCount, OtherSize>&) noexcept {}

	static void* allocate(size_t n) {
		if (n <= Size) {
			for (int i = 0; i < ElementCount; i++) {
				if (!_space.usage[i]) {
					_space.usage[i] = true;
					return _space.buffer.data() + Size * i;
				}
			}
		}
		return new int8_t[n];
	}

	static void deallocate(void* pointer, size_t n) noexcept {
		if (pointer >= _space.buffer.data() && pointer < _space.buffer.data() + Size * ElementCount) {
			int index = reinterpret_cast<int8_t*>(pointer) - _space.buffer.data();
			index /= Size;
			_space.usage[index] = false;
		} else
			delete reinterpret_cast<int8_t*>(pointer);
	}
};

template <int Size, int OtherSize>
bool operator==(const StaticAllocator<Size>&, const StaticAllocator<OtherSize>&) { return true; }
template <int Size, int OtherSize>
bool operator!=(const StaticAllocator<Size>&, const StaticAllocator<OtherSize>&) { return false; }

#endif // SCHEDULER_DUGI_HPP
