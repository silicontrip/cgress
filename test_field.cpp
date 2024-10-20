#include <iostream>
#include <chrono>
#include <thread>
#include "run_timer.hpp"

#include "field.hpp"
#include "point.hpp"

using namespace std;

int main (int argc, char* argv[])
{
	using namespace silicontrip;
	run_timer rt;

	rt.start();

	cout << "start." << endl;	


	point p1 = point(-37.813154,145.348738);
	point p2 = point(-37.813431,145.341783);
	point p3 = point(-37.80919,145.343338);
	field f1 = field(p1,p2,p3);

	cout << "point at: " << f1.point_at(1) << endl;
	cout << "geo area: " << f1.geo_area() << endl;
	cout << "geo perimeter: " << f1.geo_perimeter() << endl;
	cout << "split: " << rt.split() << endl;

	point p4 = point(-37.811022,145.342732);
	point p5 = point(-37.812462,145.344617);
	point p6 = point(-37.811708,145.345299);
	field f2 = field(p4,p5,p6);
	
	field f3 = field(p4,p5,p1);

	cout << "intersects: " << f1.intersects(f2) << " " << f1.intersects(f3) << " " << f2.intersects(f3)  << endl;
	cout << "touches: " << f1.touches(f2) << " " << f1.touches(f3) << " " << f2.touches(f3)  << endl;
	cout << "inside: " << f1.inside(f2) << " " << f1.inside(f3) << " " << f2.inside(f3)  << endl;
	cout << "layers: " << f1.layers(f2) << " " << f1.layers(f3) << " " << f2.layers(f3)  << endl;
	cout << "difference: " << f1.difference(f2) << " " << f1.difference(f3) << " " << f2.difference(f3)  << endl;

	cout << "split: " << rt.split() << endl;

	cout << "[" << f1.drawtool() << "," << f2.drawtool() << "," << f3.drawtool() << "]" << endl;

	cout << "stop: " << rt.stop() << endl;

	return 0;
}
