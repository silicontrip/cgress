#include <iostream>
#include <chrono>
#include <thread>
#include "run_timer.hpp"

using std::cout;
using std::endl;

int main ()

{
	using namespace silicontrip;
	run_timer rt;

	srand(time(NULL));
	rt.start();

	cout << "start." << endl;	

	std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 2000));

	cout << "split: " << rt.split() << endl;

	std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 2000));
	cout << "stop: " << rt.stop() << endl;

	return 0;
}
