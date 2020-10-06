# Lightweight Containers
STL comes with pretty nice containers. However, most of them are strongly disrecommended for embedded development because they bloat the code and use dynamic allocation liberally.

I have made a some containers designed to be much more lightweight at the cost of being limited in size:

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
