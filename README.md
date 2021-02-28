# Lightweight Containers
STL comes with pretty nice utility classes. However, most of them are strongly disrecommended for embedded development because they bloat the code and use dynamic allocation liberally.

I have made a some utilities designed to be much more lightweight at the cost of being limited in size:

## Coroutines
Coroutines are super useful in embedded software. They allow writing slow control logic that needs to be interrupted a lot as interruptible functions rather than long and unreadable state machines. `scheduler.hpp` (and `scheduler.cpp`) contain a utility wrapper for coroutine handles and a scheduler than can run them.

```C++
// Function declaration somewhere
Task runImportantTask() {
	while (true) {
		importantTask();
		co_await waitForMs(150);
	}
}

//...
Scheduler<16> scheduler; // Capable of holding 16 coroutines
scheduler.addTask(runImportantTask());

// main loop
scheduler.runATask(); // Runs a single coroutine if one has to run
```

Coroutines created via `Task` are dynamically allocated and are thus intended to be long term task. Anyway, once they hit `co_return`, they end and are removed from the scheduler. The scheduler has no priority and no concept of starvation. Function `waitSomeTime()` can be used instead of `waitForMs()` to run the task when no other tasks need to be executed. In `scheduler.cpp`, constant `TOLERANCE` can be set to make it execute tasks several milliseconds earlier to avoid executing tasks too late.

If a task needs to wait for another interruptible function to finish, it can use `Awaitable`.

```C++
Awaitable<int> work() {
	co_return co_await workForLong();
}
//...
int obtained = co_await work();
```

`Awaitable` uses dynamic allocation by default, but it can be prevented by giving it an explicit pool for allocation.
```C++
template <typename T>
using StaticAwaitable = Awaitable<T, StaticAllocator<16, 80>>;
```
This causes any coroutines based on `StaticAwaitable` to allocate in a buffer with space for 16 coroutines with up to 80 bytes. If there is no space or the coroutine is too large, it will use dynamic allocation instead.

## Circular Buffer
Contains two data structures built atop a circular buffer. The circular buffer is written in C-style C++ and works with raw bytes only (and thus supports only trivially copiable types). It's extremely impractical to use and thus is meant to be used only by template façades whose only purpose is to properly cast the arguments.

Its methods `size()`, `empty()` and `full()` are not impractical and are available through the façades. Because the size of the buffer is fixed, it can also be full.

### CircularQueue
A FIFO queue. Supports methods `pushBack()`, `popFront()` and `front()`. Is trivially copiable.

```C++
CircularQueue<int, 3> buffer; // Contains int, size is 3 (would be 8 if omitted)
buffer.pushBack(3);
buffer.pushBack(4);
if (!buffer.full())
  buffer.pushBack(5);
if (!buffer.empty()) {
  int front = buffer.front();
  buffer.popBack();
}
```

### CircularBuffer
An unordered map. Search is linear, insertion is constant. Trivially copiable.

```C++
struct Inside {
  float value;
  int index;
};
CircularBuffer<Inside, &Inside::index, 3> buffer; // contained, pointer to index member, size
buffer.insert({3.3, 3});
buffer.insert({4.4, 4});
int size = buffer.size();
if (buffer.has(4))
  buffer.insert({5.5, 5});
Inside noLonger = buffer.extract(3); // returns garbage if the element is missing
std::optional<Inside> maybe = buffer.tryGet(3);
Inside sure = buffer.get(4); // returns garbage if missing
std::optional<Inside> maybeRemoved = buffer.tryExtract(5);
```

## Function
Similar to `std::function`, but uses neither dynamic allocation nor virtual function calls. This restricts the maximum size of the closure it (to the size of two pointers by default) can contain and can only be used on trivially copiable types. This covers most use cases of `std::function`.

```C++
Function<void(int)> a = [&value] (int added) {
  value += added;
};
Function<void(int), 24> /* larger */ b = [&otherValue, a] (int added) {
  otherValue += added;
  a(added);
};

b(13);
b = a; // smaller can be assigned into larger
```

It can also be used as some kind of `std::bind` to capture methods:
```C++
struct Point {
  int x;
  int y;
  void increment() {
    x++;
    y++;
  }
};

Point pt = { 17, 19 };
Function<void()> c = bindMethod<&Point::increment>(&pt);
c();
```
