#include <iostream>
#include <chrono>
#include <thread>
#include "run_timer.hpp"
#include "point.hpp"

using std::cout;
using std::endl;

int main ()

{
	using namespace silicontrip;
	run_timer rt;

	rt.start();

	cout << "start." << endl;	

	point p1 = point("-37.818408,144.949842");
	cout << "p1: " << p1 <<". " << rt.split() << endl;

	point p2 = point(-37812201L,145347026L);

	cout << "p2: " << p2 <<". " << rt.split() << endl;

	cout << "dist: " << p1.s2latlng().GetDistance(p2.s2latlng()) << endl;

	point p3 = point("-37.814997","144.947705");
	cout << "p3: " << p3 <<". dist: " << p3.geo_distance_to(p1) << "km. " << rt.split() << endl;

	point p4 = point(-37.811022,145.342732);
	cout << "p4: " << p4 <<". dist:" << p4.geo_distance_to(p1)<<"km. " << rt.split() << endl;


	cout << "stop: " << rt.stop() << endl;

	return 0;
}
