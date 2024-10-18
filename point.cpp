
#include "point.hpp"

namespace silicontrip {

long point::lat_e6() const { return latlng.lat().e6(); }
long point::lng_e6() const { return latlng.lng().e6(); }
double point::lat_double() const { return (double)latlng.lat().e6(); }
double point::lng_double() const { return (double)latlng.lng().e6(); }
double point::lat() const { return latlng.lat().degrees(); }
double point::lng() const { return latlng.lng().degrees(); }
S2LatLng point::s2latlng() const { return latlng; }
double point::lat_radians() const { return latlng.lat().radians(); }
double point::lng_radians() const { return latlng.lng().radians(); }
S2Point point::s2point() const { return latlng.ToPoint(); }


point::point(S2Point p) { latlng = S2LatLng(p); }
point::point(long la, long ln) { latlng = S2LatLng::FromE6(la,ln); }
point::point(std::string ld) 
{
        int f1 = ld.find(",", 0);
	double lat;
	double lng;
        std::istringstream(ld.substr(0, f1)) >> lat;
	std::istringstream(ld.substr(f1+1)) >> lng;

	latlng = S2LatLng::FromDegrees(lat,lng);
}
point::point(std::string la, std::string ln) {  
	double lat;
	double lng;
	std::istringstream(la) >> lat;
	std::istringstream(ln) >> lng;
	latlng = S2LatLng::FromDegrees(lat,lng);
}
point::point(double la, double ln) { latlng = S2LatLng::FromDegrees(la,ln); } 
point::point(point& p) { latlng = S2LatLng(p.s2latlng().ToPoint()); }
point::~point() { ; }
		
std::string point::toString() const { return "" + std::to_string(lat_e6() / 1000000.0) + "," + std::to_string(lng_e6() / 1000000.0); }

bool point::operator==(const point& o) const { return latlng == o.latlng; }

point point::inverse() const {
	if (lng_radians() > 0) 
		return point(-this->lat_e6(), this->lng_e6() - 180000000);
	else
		return point(-this->lat_e6(), this->lng_e6() + 180000000);
}
S1Angle point::distance_to(point& p) const { return latlng.GetDistance(p.s2latlng()); }
double point::geo_distance_to(point& p) const { return distance_to(p).radians() * earth_radius; }
//S1Angle point::angle_between(point& p1, point& p2) 
//S1Angle point::bearing_to(point& p) 
		

}

std::ostream& operator<<(std::ostream& os, const silicontrip::point& p)
{
    os << p.toString();
    return os;
}
