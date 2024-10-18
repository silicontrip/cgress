#include "line.hpp"

using std::string;
using std::vector;

namespace silicontrip {

long line::d_lat_e6() const { return d_point.s2latlng().lat().e6(); } 
long line::d_lng_e6() const { return d_point.s2latlng().lng().e6(); } 
long line::o_lat_e6() const { return o_point.s2latlng().lat().e6(); }
long line::o_lng_e6() const { return o_point.s2latlng().lng().e6(); }

double line::d_lat_double() const { return (double)d_point.s2latlng().lat().e6(); }
double line::d_lng_double() const { return (double)d_point.s2latlng().lng().e6(); }
double line::o_lat_double() const { return (double)o_point.s2latlng().lat().e6(); }
double line::o_lng_double() const { return (double)o_point.s2latlng().lng().e6(); }

S2LatLng line::d_s2latlng() const { return d_point.s2latlng(); }
S2LatLng line::o_s2latlng() const { return o_point.s2latlng(); }

line::line() { ; }
line::line(point d, point o): d_point(d), o_point(o)
{ 
	//d_point.FromE6(d.lat().e6(),d.lng().e6());
	//o_point.FromE6(o.lat().e6(),o.lng().e6());

	o_s2Point = o_point.s2latlng().ToPoint();
	d_s2Point = d_point.s2latlng().ToPoint();
	// crosser.S2CopyingEdgeCrosser(d_point.ToPoint(),o_point.ToPoint());
}
line::line(long dla, long dlo, long ola, long olo): d_point(dla,dlo), o_point(ola,olo)
{ 
	//d_point.FromE6(dla,dlo); 
	//o_point.FromE6(ola,olo); 

	//crosser.S2EdgeCrosserBase(d_point.ToPoint(),o_point.ToPoint());
}

bool line::operator==(const line& l) const { return (l.o_point==o_point && l.d_point==d_point) || ( l.o_point==d_point && l.d_point==o_point); }
bool line::found(vector<line> la) const
{
	for (line li : la)
	{
		if (*this == li)
			return true;
	}
	return false;
}
bool line::intersects(const line& l) const
{
	return S2::CrossingSign(o_s2Point,d_s2Point,l.o_s2Point,l.d_s2Point) == 1;
}
bool line::intersects(vector<line> l) const
{
	for (line li : l)
	{
		if (S2::CrossingSign(o_s2Point,d_s2Point,li.o_s2Point,li.d_s2Point) == 1)
			return true;
	}
	return false;
}	
bool line::intersect_or_equal(line l) const { return great_circle_intersection_type(l) != 2; }
bool line::intersect_or_equal(vector<line> l) const
{
	for (line li : l)
	{
		if (great_circle_intersection_type(li) != 2)
			return true;
	}
	return false;
}

double line::geo_distance() const { return S2Earth::ToMeters(o_point.s2latlng().GetDistance(d_point.s2latlng())) / 1000.0; }
double line::geo_distance(const point& p) const 
{
	S2LatLng on_line = S2LatLng(S2::Project(p.s2latlng().ToPoint(),o_point.s2latlng().ToPoint(),d_point.s2latlng().ToPoint()));
	return S2Earth::ToMeters(on_line.GetDistance(p.s2latlng())) / 1000.0;
}

std::string line::to_string() const
{
	return "" + o_point.to_string() + "-" + d_point.to_string();
}

bool line::has_point(const point& p) const { return p==d_point || p==o_point; }

S2Point line::normal() const
{
	S2Point N = S2::RobustCrossProd(o_s2Point,d_s2Point);
	return N.Normalize();
}

int line::point_on (S2Point D, line l) const
{
	S2Point V = normal();
	S2Point U = l.normal();

	S2Point S1 = S2::RobustCrossProd(o_s2Point,V);
	S2Point S2 = S2::RobustCrossProd(d_s2Point,V);
	S2Point S3 = S2::RobustCrossProd(l.o_s2Point,U);
	S2Point S4 = S2::RobustCrossProd(l.d_s2Point,U);

	double p[] = {-S1.DotProd(D),S2.DotProd(D),-S3.DotProd(D),S4.DotProd(D)};

	int zero=0;
	int count=0;

	for (int i =0; i<4; i++)
	{
	//System.out.println("Zero: " + p[i]);

		if (fabs(p[i]) < eps)
			zero++;
		else
			count += (p[i] > 0) ? 1 : ((p[i] < 0) ? -1 : 0);
	}

	if (zero==4)
		return 0;

	if (count==-4 || count==4)
		return 1;
                
	if (zero>0)
		return 3;

	return 2;

}

int line::great_circle_intersection_type(line l) const
{
	S2Point int_point = S2::GetIntersection(o_s2Point,d_s2Point,l.o_s2Point,l.d_s2Point);
	return point_on(int_point,l);
}

}

std::ostream& operator<<(std::ostream& os, const silicontrip::line& l)
{
    os << l.to_string();
    return os;
}