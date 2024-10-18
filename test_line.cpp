#include <iostream>
#include <chrono>
#include <thread>
#include "run_timer.hpp"
#include "point.hpp"
#include "line.hpp"

using std::cout;
using std::endl;

int main ()

{
	using namespace silicontrip;
	run_timer rt;

	rt.start();

	cout << "start." << endl;	

	point p1 = point(-37.818408,144.949842);
	point p2 = point(-37.812201,145.347026);
	point p3 = point(-37.814997,144.947705);
	point p4 = point(-37.811022,145.342732);

	line l1 = line (p1,p2);
	line l2 = line (p1,p3);
	line l3 = line (p1,p4);
	line l4 = line (p2,p3);
	line l5 = line (p2,p4);
	line l6 = line (p3,p4);

	cout << "l1: " << l1 <<". dist:" << l1.geo_distance() << ". " << rt.split() << endl;
	cout << "l2: " << l2 <<". dist:" << l2.geo_distance() << ". " << rt.split() << endl;
	cout << "l3: " << l3 <<". dist:" << l3.geo_distance() << ". " << rt.split() << endl;
	cout << "l4: " << l4 <<". dist:" << l4.geo_distance() << ". " << rt.split() << endl;
	cout << "l5: " << l5 <<". dist:" << l5.geo_distance() << ". " << rt.split() << endl;
	cout << "l6: " << l6 <<". dist:" << l6.geo_distance() << ". " << rt.split() << endl;

	cout << "l1-l2: " << l1.intersects(l2) << ". " << rt.split() << endl;
	cout << "l1-l2: " << l1.great_circle_intersection_type(l2) << ". " << rt.split() << endl;
	cout << "l3-l4: " << l3.intersects(l4) << ". " << rt.split() << endl;
	cout << "l3-l4: " << l3.great_circle_intersection_type(l4) << ". " << rt.split() << endl;
	cout << "l5-l6: " << l5.intersects(l6) << ". " << rt.split() << endl;
	cout << "l5-l6: " << l5.great_circle_intersection_type(l6) << ". " << rt.split() << endl;
	
	cout << "same: " << l1.great_circle_intersection_type(l1) << ". " << rt.split() << endl;

	cout << "stop: " << rt.stop() << endl;

	return 0;
}
