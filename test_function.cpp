//usr/bin/g++ --std=c++17 -Wall $0 -o ${o=`mktemp`} && exec $o $*
#include "function.hpp"
#include <iostream>

struct Point {
	int x;
	int y;
	void print() const {
		std::cout << "Point " << x << " " << y << std::endl;
	}
	void increment() {
		x++;
		y++;
	}
};

int main() {
	Function<void(int), 8> a = [](int printed) {
		std::cout << "Printing " << printed << std::endl;
	};
	Function<void(int)> b = [a] (int printed) {
		int extra = 5;
		a(printed + extra);
	};

	Point pt = { 17, 19 };
	Function<void()> c = bindMethod<&Point::print>(&pt);
	Function<void()> d = [pt] { pt.print(); };

	a(3);
	b(4);
	b = a;
	b(4);
	// a = b; // static assert fails

	c();
	d();

	d = bindMethod<&Point::increment>(&pt);
	d();
	pt.print();
	d = [] { std::cout << "Not printing anything" << std::endl; };
	d();

	std::cout << "Sizes " << sizeof(a) << " " << sizeof(b) << std::endl;
}
