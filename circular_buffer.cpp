#include "circular_buffer.hpp"
#include <cstring>

CircularBufferImpl::CircularBufferImpl(uint8_t* data, short int capacity, short int elementSize) :
	_data(data),
	_capacity(capacity),
	_elementSize(elementSize) {
  }

void CircularBufferImpl::pushBack(const void* data) {
  if (_lastInserted >= 0)
	_lastInserted = (_lastInserted + 1) % _capacity;
  else {
	_lastInserted = _firstInserted;
  }
  memcpy(_data + _lastInserted * _elementSize, data, size_t(_elementSize));
}

void* CircularBufferImpl::front() const {
  return (_data + _firstInserted * _elementSize);
}

void CircularBufferImpl::popFront() {
  bool emptied = (_lastInserted == _firstInserted);
  _firstInserted = (_firstInserted + 1) % _capacity;
  if (emptied)
	_lastInserted = -1;
}

bool CircularBufferImpl::find(uint8_t* sequence, int from, int till, uint8_t* copyLocation, bool erase) {
  if (_lastInserted == -1)
	return false;
  for (int i = _firstInserted; i <= _lastInserted; i = (i + 1) % _capacity) {
	bool matches = true;
	int offset = i * _elementSize + from;
	for (int j = 0; j < till - from; j++)
	  if (sequence[j] != _data[offset + j]) {
		matches = false;
		break;
	  }
	if (matches) {
	  if (copyLocation) {
		for (int j = 0; j < _elementSize; j++)
		  copyLocation[j] = _data[i * _elementSize + j];
	  }
	  if (erase) {
		if (_firstInserted != _lastInserted) {
		  for (int j = 0; j < _elementSize; j++)
			_data[i * _elementSize + j] = _data[_firstInserted * _elementSize + j];
		}
		popFront();
	  }
	  return true;
	}
  }
  return false;
}

bool CircularBufferImpl::full() const {
	if (_lastInserted == -1)
	  return false;
  return ((_lastInserted + 1) % _capacity == _firstInserted);
}

int CircularBufferImpl::size() const {
  if (_lastInserted == -1)
	return 0;
  return (_lastInserted + _capacity - _firstInserted) % _capacity + 1;
}

bool CircularBufferImpl::empty() const {
  return _lastInserted == -1;
}
