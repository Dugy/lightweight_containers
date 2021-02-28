#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H
#include <array>
#include <optional>

class CircularBufferImpl {
	uint8_t* _data;
	short int _capacity;
	short int _elementSize;
	short int _firstInserted = 0;
	short int _lastInserted = -1; // -1 if empty

public:
	CircularBufferImpl(uint8_t* data, short int capacity, short int elementSize);

	void pushBack(const void* data);

	void* front() const;

	void popFront();

	bool find(uint8_t* sequence, int from, int till, uint8_t* copyLocation, bool erase);

	bool full() const;

	int size() const;

	bool empty() const;
};

template <typename T, int CAPACITY = 8>
class CircularQueue : private CircularBufferImpl {
	std::array<T, CAPACITY> data;
public:
	CircularQueue() : CircularBufferImpl(reinterpret_cast<uint8_t*>(data.data()), CAPACITY, sizeof(T)) {
	}

	void pushBack(const T& element) {
		CircularBufferImpl::pushBack(&element);
	}

	T& front() const {
		return *reinterpret_cast<T*>(CircularBufferImpl::front());
	}

	using CircularBufferImpl::popFront;
	using CircularBufferImpl::full;
	using CircularBufferImpl::size;
	using CircularBufferImpl::empty;
	constexpr static int capacity = CAPACITY;
};

template <typename T, auto Index, int CAPACITY = 8>
class CircularBuffer : private CircularBufferImpl {
	std::array<T, CAPACITY> data;
	using indexType = decltype(std::declval<T>().*Index);
	static inline size_t indexOffset() {
		return reinterpret_cast<size_t>(&(reinterpret_cast<T*>(0)->*Index));
	}
	static inline size_t indexEnd() {
		return indexOffset() + sizeof(indexType);
	}
public:
	CircularBuffer() : CircularBufferImpl(reinterpret_cast<uint8_t*>(&data), CAPACITY, sizeof(T)) {
	}

	void insert(const T& element) {
		CircularBufferImpl::pushBack(&element);
	}

	bool has(indexType index) const {
		return const_cast<CircularBuffer<T, Index, CAPACITY>*>(this)->find(reinterpret_cast<uint8_t*>(&index), indexOffset(), indexEnd(), nullptr, false);
	}

	T get(indexType index) {
		T got;
		find(reinterpret_cast<uint8_t*>(&index), indexOffset(), indexEnd(), reinterpret_cast<uint8_t*>(&got), false);
		return got;
	}

	std::optional<T> tryGet(indexType index) {
		T got;
		if (!find(reinterpret_cast<uint8_t*>(&index), indexOffset(), indexEnd(), reinterpret_cast<uint8_t*>(&got), false))
			return std::nullopt;
		return got;
	}

	T extract(indexType index) {
		T got;
		find(reinterpret_cast<uint8_t*>(&index), indexOffset(), indexEnd(), reinterpret_cast<uint8_t*>(&got), true);
		return got;
	}

	std::optional<T> tryExtract(indexType index) {
		T got;
		if (!find(reinterpret_cast<uint8_t*>(&index), indexOffset(), indexEnd(), reinterpret_cast<uint8_t*>(&got), true))
		return std::nullopt;
		return got;
	}

	using CircularBufferImpl::full;
	using CircularBufferImpl::size;
	using CircularBufferImpl::empty;
	constexpr static int capacity = CAPACITY;
};



#endif // CIRCULAR_BUFFER_H
