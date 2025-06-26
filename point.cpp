
#include "point.hpp"
#include "line.hpp"

using std::istringstream;

namespace silicontrip {

point::point(std::string ld) 
{
    int f1 = ld.find(",", 0);
	double lat;
	double lng;
    istringstream(ld.substr(0, f1)) >> lat;
	istringstream(ld.substr(f1+1)) >> lng;

	latlng = S2LatLng::FromDegrees(lat,lng);
}
point::point(std::string la, std::string ln) {  
	double lat;
	double lng;
	istringstream(la) >> lat;
	istringstream(ln) >> lng;
	latlng = S2LatLng::FromDegrees(lat,lng);
}
point::point(S2Point sp) { latlng = S2LatLng(sp); }
point::point(S2LatLng ll) { latlng = ll; }
point::point(long la, long ln) { latlng = S2LatLng::FromE6(la,ln); }
point::point(double la, double ln) { latlng = S2LatLng::FromDegrees(la,ln); }
point::point(const point& p) { latlng = S2LatLng(p.latlng.lat(),p.latlng.lng()); }
point::point() { ; }

S2LatLng point::s2latlng() const { return latlng; }
void point::set_s2latlng(S2LatLng ll) { latlng = ll; }
std::string point::to_string() const { return "" + std::to_string(latlng.lat().e6() / 1000000.0) + "," + std::to_string(latlng.lng().e6() / 1000000.0); }


point point::inverse() const {
	if (latlng.lng().radians() > 0) 
		return point(-latlng.lat().e6(), latlng.lng().e6() - 180000000L);
	else
		return point(-latlng.lat().e6(), latlng.lng().e6() + 180000000L);
}
// S2Library changed its Earth radius value
double point::geo_distance_to(const point& p) const { return latlng.GetDistance(p.s2latlng()).radians() * earth_radius; }
S1Angle point::ang_distance_to(const point& p) const { return latlng.GetDistance(p.s2latlng()); }

//S1Angle point::angle_between(point& p1, point& p2) 
//S1Angle point::bearing_to(point& p) 

int point::count_links(std::vector<line> l) const
{
	int count  = 0;
	for (line li : l)
	{
		if (li.d_s2latlng() == latlng || li.o_s2latlng() == latlng )
		{
			count++;
		}
	}
	return count;
}

int point::count_dlinks(std::vector<line> l) const
{
	int count  = 0;
	for (line li : l)
	{
		if (li.d_s2latlng() == latlng )
		{
			count++;
		}
	}
	return count;
}

int point::count_olinks(std::vector<line> l) const
{
	int count  = 0;
	for (line li : l)
	{
		if (li.o_s2latlng() == latlng )
		{
			count++;
		}
	}
	return count;
}

point point::Invalid()
{
	return point(S2LatLng::Invalid());
}


bool point::is_valid() const
{
	return latlng.is_valid();
}

S1Angle point::bearing_to(point p) const
{
	double dlon = p.latlng.lng().radians() - latlng.lng().radians();
	double x = cos(latlng.lat().radians()) * sin(p.latlng.lat().radians()) - sin(latlng.lat().radians()) * cos(p.latlng.lat().radians()) * cos(dlon);
	double y = sin(dlon) * cos(p.latlng.lat().radians());

	double brng = atan2(y, x);
	S1Angle b = S1Angle::Radians(brng);

	return b;
}

point point::project_to(S1Angle distance, S1Angle bearing) const
{
	double la2 =  asin(sin (latlng.lat().radians()) * cos(distance.radians()) + cos(latlng.lat().radians()) * sin (distance.radians()) * cos(bearing.radians()));
	double lo2 = latlng.lng().radians() + atan2(sin(bearing.radians()) * sin(distance.radians()) * cos (latlng.lat().radians()) , cos (distance.radians()) - sin(latlng.lat().radians()) * sin (la2));

	S2LatLng ll = S2LatLng::FromRadians(la2,lo2);

	return point(ll);
}

bool point::operator==(const point& p) const { return p.latlng == latlng; }
bool point::operator<(const point& p) const
{
    if (latlng.lat().radians() != p.s2latlng().lat().radians())
        return latlng.lat().radians() < p.s2latlng().lat().radians();
    return latlng.lng().radians() < p.s2latlng().lng().radians();
}
}

std::ostream& operator<<(std::ostream& os, const silicontrip::point& p)
{
    os << p.to_string();
    return os;
}
