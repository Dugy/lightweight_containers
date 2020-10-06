#ifndef FUNCTIONAL_H
#define FUNCTIONAL_H
#include <cstring>

template <typename T, size_t bufferSize = sizeof(void*) * 2>
class Function;

template <typename Returned, size_t bufferSize, typename... Args>
class Function<Returned(Args...), bufferSize> {
  Returned (*_called)(const void*, Args...) = nullptr;
  uint8_t _data[bufferSize];

public:
  Function() = default;

  template <typename T, decltype(Returned(std::declval<T>()(std::declval<Args>()...)))* = nullptr>
  Function(const T& set) {
    static_assert(std::is_trivially_copyable_v<T>, "Only trivially copyable types can be captured.");
    static_assert(sizeof(T) <= bufferSize, "Class too large to fit");
    _called = [](const void* data, Args... args) {
      return reinterpret_cast<const T*>(data)->operator()(args...);
    };
    new (_data) T(set);
  }

  template <size_t otherBufferSize, std::enable_if_t<otherBufferSize != bufferSize>* = nullptr>
  Function(const Function<Returned(Args...), otherBufferSize>& other) {
    static_assert(otherBufferSize < bufferSize, "Cannot create a Function type from a larger Function type");
    memcpy(this, &other, sizeof(Function<Returned(Args...), otherBufferSize>));
  }

  Returned operator()(Args... args) const {
    return _called(reinterpret_cast<const void*>(_data), args...);
  }

  operator bool() const {
    return (_called != nullptr);
  }

  void reset() {
    _called = nullptr;
  }
};

template <typename T, auto Method, typename Returned, typename... Args>
static auto bindMethodImpl(T* object, Returned (T::*)(Args...)) {
  return [object] (Args... args) {
    return (object->*Method)(args...);
  };
}

template <typename T, auto Method, typename Returned, typename... Args>
static auto bindMethodImpl(T* object, Returned (T::*)(Args...) const) {
  return [object] (Args... args) {
    return (object->*Method)(args...);
  };
}

template <auto method, typename T>
static auto bindMethod(T* object) {
  return bindMethodImpl<T, method>(object, method);
}

#endif // FUNCTIONAL_H
