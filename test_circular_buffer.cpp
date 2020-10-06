#include "circular_buffer.hpp"
#include <iostream>

int main() {

  std::cout << "Queue" << std::endl;

  CircularQueue<int, 3> buffer;
  buffer.pushBack(3);
  buffer.pushBack(4);
  std::cout << "Full (2/3)? " << buffer.full() << std::endl;
  buffer.pushBack(5);
  std::cout << "Full (3/3)? " << buffer.full() << std::endl;

  std::cout << "Front (should be 3) " << buffer.front() << std::endl;
  buffer.popFront();
  buffer.popFront();
  std::cout << "Front (should be 5) " << buffer.front() << std::endl;
  std::cout << "Empty (1/3)? " << buffer.empty() << std::endl;
  buffer.popFront();
  std::cout << "Full (0/3)? " << buffer.full() << std::endl;
  std::cout << "Empty (0/3)? " << buffer.empty() << std::endl;

  std::cout << std::endl;

  CircularQueue<float, 2> buffer2;
  buffer2.pushBack(3.3);
  buffer2.pushBack(4.4);
  buffer2.popFront();
  buffer2.pushBack(5.5);
  std::cout << "Front (should be 4.4) " << buffer2.front() << std::endl;
  std::cout << "Size (2/2) " << buffer2.size() << std::endl;
  buffer2.popFront();
  std::cout << "Size (1/2) " << buffer2.size() << std::endl;
  buffer2.popFront();
  std::cout << "Size (0/2) " << buffer2.size() << std::endl;

  std::cout << std::endl;
  std::cout << "Buffer" << std::endl;
  struct Inside {
	float value;
	int index;
  };
  CircularBuffer<Inside, &Inside::index, 3> buffer3;
  buffer3.insert({3.3, 3});
  buffer3.insert({4.4, 4});
  std::cout << "Size is (should be 2) " << buffer3.size() << std::endl;
  std::cout << "Is 3 there? (should be) " << buffer3.has(3) << std::endl;
  std::cout << "Is 5 there? (shouldn't be) " << buffer3.has(5) << std::endl;
  std::cout << "What is on 4? (4.4) " << buffer3.get(4).value << std::endl;
  std::cout << "Extracting 3. (3.3) " << buffer3.extract(3).value << std::endl;
  std::cout << "Is 3 there? (shouldn't be) " << buffer3.has(3) << std::endl;
  std::cout << "Size is (should be 1) " << buffer3.size() << std::endl;
  std::cout << "Extracting 4. (4.4) " << buffer3.extract(4).value << std::endl;
  std::cout << "Size is (should be 0) " << buffer3.size() << std::endl;
  std::cout << "Can get 4 now? (shouldn't) " << bool(buffer3.tryGet(4)) << std::endl;
}
