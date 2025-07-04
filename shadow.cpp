#include "run_timer.hpp"
#include "arguments.hpp"
#include "team_count.hpp"
#include "portal_factory.hpp"
#include "link_factory.hpp"
#include "field_factory.hpp"
#include "draw_tools.hpp"
#include "link.hpp"
#include "field.hpp"

using namespace std;
using namespace silicontrip;

struct layer {
	S1Angle angle;
	team_count level;
	enum ingressteam team;
	int start; 
	int end;
	vector<point> points;
};

struct dist_compare
{
    dist_compare(const point& _p) : pp(_p) {}

    bool operator()(const point& a, const point& b) const
    {
        //return dist(p, lhs) < dist(p, rhs);
		return a.s2latlng().GetDistance(pp.s2latlng()) < b.s2latlng().GetDistance(pp.s2latlng());

    }

private:
    point pp;
};

class shadow {

private:

	run_timer rt;
	vector<silicontrip::link> links;
	unordered_map<double,struct layer> layers;
	vector<double> intervals;
	portal pp;
	S1Angle radius;
	//int strength;
	team_count strength;

	struct layer make_seg(S1Angle a, team_count s, bool se, point l, team_count limit, enum ingressteam originteam) const;
	vector<point> get_intersections(const vector<silicontrip::link>& a, line l) const;
	team_count count_intersections(const vector<silicontrip::link>& a, line l) const;

public:
	shadow(const vector<silicontrip::link>& a, const portal& p, double r, team_count s, run_timer& run);
	void prep();
	draw_tools make_layer(draw_tools d, team_count layer, string colour);
	int max();
	//bool dist_compare(const point& a, const point& b);

};

shadow::shadow(const vector<silicontrip::link>& a, const portal& p, double r, team_count s, run_timer& run)
{
	links = a;
	pp = p;
	
	radius = S1Angle::Radians(r / point::earth_radius);
	strength = s;

	rt = run;
}

vector<point> shadow::get_intersections(const vector<silicontrip::link>& a, line l) const
{
	vector<point> r;
	// we assume o_point is at a portal

	r.push_back(l.get_o_point());
	S2Point al = l.o_s2point();
	S2Point bl = l.d_s2point();
	S2EdgeCrosser ec(&al, &bl);
	for (silicontrip::link li : a)
	{
		if (!(pp == li.get_d_point() || pp == li.get_o_point()))
		{
			S2Point cl = li.o_s2point();
			S2Point dl = li.d_s2point();

			if (ec.CrossingSign(&cl,&dl) == 1 )
			{
				S2Point c = S2::GetIntersection(al,bl,cl,dl);
				point p =point(c);

				r.push_back(p);  
			}
		}
	}

	r.push_back(l.get_d_point());

	return r;
}

team_count shadow::count_intersections(const vector<silicontrip::link>& a, line l) const
{
	team_count tc(0,0,0);
	//int count = 0;
	S2Point al = l.o_s2point();
	S2Point bl = l.d_s2point();
	S2EdgeCrosser ec(&al, &bl);
	for (silicontrip::link li : a)
	{
		if (!(pp == li.get_d_point() || pp == li.get_o_point()))
		{
			S2Point cl = li.o_s2point();
			S2Point dl = li.d_s2point();
			if (ec.CrossingSign(&cl,&dl) == 1)
			{
				//count++;
				tc.inc_team_enum(li.get_team_enum());
			}
		}
	}
	return tc;
}

vector<point> filter_points(const vector<pair<point,enum ingressteam>>& pts, team_count limit)
{
	team_count count(0,0,0);
	vector<point> result;
	result.push_back(pts[0].first);
	for (int i =1; i < pts.size(); i++)
	{
		pair<point,enum ingressteam> pi = pts[i];
		result.push_back(pi.first);
		count.inc_team_enum(pi.second);
		if (count > limit)
			break;
	}
	return result;
}

struct layer shadow::make_seg(S1Angle a, team_count s, bool se, point l, team_count limit, enum ingressteam originteam) const
{
	struct layer tl;

	tl.angle = a;
	tl.level = s;
	tl.team = originteam;
	if (se)
	{
		tl.start = 1;
		tl.end = 0;
	}
	else
	{
		tl.end = 1;
		tl.start = 0;
	}
	point epoint = pp.project_to(radius,a);
	//cerr << "point: " << l << endl;
	line coli = line (epoint,l);
	// find all intersections 
	// inter.push_back()
	vector<point> inter = get_intersections(links,coli);
	// sort by distance from centre
	// and filter after max team_count crossings.

	sort(inter.begin(), inter.end(), dist_compare(pp));

	// inter.push_back(epoint);
	tl.points = inter;

	return tl;
}

void shadow::prep()
{
	for (silicontrip::link li : links)
	{
		if (!(pp == li.get_d_point() || pp == li.get_o_point()))
		{
			line lo = line(pp,li.get_o_point());
			line ld = line(pp,li.get_d_point());

			//
			team_count seco = count_intersections(links,lo);
			team_count secd = count_intersections(links,ld);

			//cerr << "str: " << strength << " o links: " << seco << " d links: " << secd << endl;

			if (seco <= strength || secd <= strength) 
			{
				S1Angle o = pp.bearing_to(li.get_o_point());
				S1Angle d = pp.bearing_to(li.get_d_point());
			// check for 360 degree crossing.
				bool ostart = false;
				double ang = d.degrees() - o.degrees();

				//cerr <<"p1: " << li.get_o_point() << " p2: " << li.get_d_point() <<   " ang: " << ang << endl;

				if ( ang < -180)
					ostart = true;
				else if ( ang < 0)
					ostart = false;
				else if (ang >= 180)
					ostart = false;
				else if ( ang >= 0)
					ostart = true;

				//check if o_point less than radius
				if (seco <= strength)
				{
					//cerr << "== seco " << o.degrees() << " " << seco << " ==" << endl;

					if (layers.count(o.degrees())==0)
					{
						struct layer tl = make_seg(o,seco,ostart,li.get_o_point(),strength,li.get_team_enum());
						layers[o.degrees()]=tl;
					} else {

						if (ostart)
						{
							layers[o.degrees()].start ++;
						}
						else
						{
							layers[o.degrees()].end ++;
						}
					}
				}

				//check if d_point less than radius
				if (secd <= strength)
				{
					//cerr << "== secd " << d.degrees() << " " << secd << " ==" << endl;
					if (layers.count(d.degrees())==0)
					{
						struct layer tl = make_seg(d,secd,!ostart,li.get_d_point(),strength,li.get_team_enum());
						layers[d.degrees()]=tl;
					} else {
						if (!ostart)
						{
							layers[d.degrees()].start++;
						}
						else
						{
							layers[d.degrees()].end ++;
						}
					}
				}
			}
		}
	}
	// insert regular interval lines.

	int deginc = 10;

	for (double dego = -180; dego < 180; dego += deginc)
	{
		// need to handle the case where the radius boundary crosses a link

		// make radius intersections with links
		S1Angle a1 = S1Angle::Degrees(dego);
		S1Angle a2 = S1Angle::Degrees(dego+deginc);
		point opoint = pp.project_to(radius,a1);
		point dpoint = pp.project_to(radius,a2);
		line li = line (dpoint,opoint);

		vector<point> inter = get_intersections(links,li);
		// sort by distance from centre
		// and filter after max team_count crossings.

		sort(inter.begin(), inter.end(), dist_compare(opoint));

		// inter.push_back(epoint);

		for (point ip : inter)
		{

			double deg = pp.bearing_to(ip).degrees();

			if (layers.count(deg)==0)
			{
				S1Angle a = S1Angle::Degrees(deg);
				point epoint = pp.project_to(radius,a);
				line l =line(epoint,pp); // dumb I made the constructor d_point, o_point.
				team_count str = count_intersections(links,l);

				//vector<pair<point,enum ingressteam>> interi = get_intersections(links,l,UNKNOWN);
				//sort(interi.begin(), interi.end(), dist_compare(pp));

				//vector<point> po = filter_points(interi,strength);

				if (str <= strength)
				{
					struct layer tl;

					tl.angle = a;
					tl.level = str;

					tl.start = 1;
					tl.end = 1;

					vector<point> po;

					po.push_back(pp);
					po.push_back(epoint);

					tl.points = po;

					layers[deg] = tl;
				}
			}
		}

	}

	intervals.reserve(layers.size());
	for (pair<double,struct layer> ll : layers)
	{
		intervals.push_back(ll.first);					
	}

	std::sort(intervals.begin(), intervals.end());


}

int shadow::max()
{
	int lmax = 0;
	for (double deg : intervals)
	{
		struct layer tl = layers[deg];
		// cerr << tl.level << endl;
		if (tl.level.max() > lmax)
			lmax = tl.level.max();
	}
	return lmax;
}

draw_tools shadow::make_layer (draw_tools d, team_count layer, string colour)
{

	vector<point> loop;
	for (double deg : intervals)
	{
		struct layer tl = layers[deg];

		if (tl.level <= layer)
		{
		//cerr << "layer: " << layer << " " << deg << " : " << tl.level << endl;
			// apply layer limit here... since point filtering didn't work.
			// I wonder if this renders filter_points useless.
			int layermax =0;
			if (tl.team == ENLIGHTENED)
				layermax = layer.get_enlightened();
			if (tl.team == RESISTANCE)
				layermax = layer.get_resistance();
			if (tl.team == NEUTRAL)
				layermax = layer.get_neutral();

			int thislayer = (layermax + 1) - tl.level.max();
			int endpoints = tl.points.size() -1;

			int thisstart = layer.max() + 1 - tl.level.max();
			int thisend = layer.max() + 1 - tl.level.max();

			if (tl.start>0)
				thisstart = thislayer - tl.start;
			if (tl.end>0)
				thisend = thislayer - tl.end;

			if (thisstart < 0)
				thisstart = 0;

			if (thisend < 0)
				thisend = 0;

			if (thisstart> endpoints)
				thisstart = endpoints;

			if (thisend> endpoints)
				thisend = endpoints;

			// check that point isn't further than radius.
			if (pp.ang_distance_to(tl.points[thisend]) <= radius)
				loop.push_back(tl.points[thisend]);
			if (pp.ang_distance_to(tl.points[thisstart]) <= radius)
				loop.push_back(tl.points[thisstart]);
		}

	}
	d.set_colour(colour);
	d.add(loop);

	return d;
}

void print_usage()
{
		cerr << "Usage:" << endl;
		cerr << "shadow -r <range km> [options] <portal>" << endl;
		cerr << endl;
		cerr << "Options:" << endl;
		cerr << " -E <number>       Limit number of Enlightened Blockers" << endl;
		cerr << " -R <number>       Limit number of Resistance Blockers" << endl;
		cerr << " -N <number>       Limit number of Machina Blockers" << endl;
		cerr << " -r <number>       Extend range of links to this distance km." << endl;
		cerr << " -S                Output each level shadow as separate drawtools." << endl;
		cerr << " -C <#colour>      Drawtools colour." << endl;
}

int main (int argc, char* argv[])
{
	run_timer rt;
	double rad = 0.0;
	vector<silicontrip::link> links;
	portal pp;
	bool sep = false;
	bool autocolour = true;
	string colour;
	int hexcolour=16777215;

	arguments ag(argc,argv);
	ag.add_req("r","radius",true); // maximum links
	// ag.add_req("s","strength",true); // number of links to cross (may extend this to faction)
	ag.add_req("E","enlightened",true); // max enlightened blockers
	ag.add_req("R","resistance",true); // max resistance blockers
	ag.add_req("N","machina",true); // max machina blockers
	ag.add_req("S","separate",false);
	ag.add_req("h","help",false);
	ag.add_req("C","colour",true); // drawtools colour

	if (!ag.parse() || ag.has_option("h"))
	{
		print_usage();
		exit(1);
	}

	team_count tc = team_count(ag.get_option_for_key("E"),ag.get_option_for_key("R"),ag.get_option_for_key("N"));

	if (tc.dont_care())
		tc = team_count(0,0,0);

	if (ag.has_option("r"))
		rad = ag.get_option_for_key_as_double("r");

	if (rad == 0.0)
	{
		cerr << "Need to specify range." << endl;
		exit(1);
	}

	if (ag.has_option("S"))
		sep = true;

	if (ag.has_option("C"))
	{
		colour = ag.get_option_for_key("C");
		autocolour = false;
	}

	if (ag.argument_size() != 1) {
		cerr << "Must specify one portal." << endl;
		exit(1);
	}

	cerr << "== Reading links and portals ==" << endl;
	rt.start();

	portal_factory* pf = portal_factory::get_instance();
	link_factory* lf = link_factory::get_instance();

    vector<portal> pps = pf->cluster_from_description(ag.get_argument_at(0));

	if (pps.size() != 1) {
		cerr << "Must specify only one portal." << endl;
		exit(1);
	}

	pp = pps[0];

	S2Point loc = pp.s2latlng().ToPoint();
	S1Angle range = S1Angle::Radians(rad / point::earth_radius);
	S2Cap creg = S2Cap(loc,range);

	S2LatLngRect bound = creg.GetRectBound();

	unordered_map<string,silicontrip::link> ll = lf->get_all_links();
	links = lf->links_in_rect(bound,ll);

	shadow ss(links,pp,rad,tc,rt);

	cerr << "preparing segments." << endl;
	ss.prep();

	cerr << "segments prepared in " << rt.split() << " seconds." << endl; 

	cerr << "layers generated: " << ss.max() << endl;

	draw_tools dt;

	for (int l=0; l <= ss.max(); l++ )
	{
		// make a count up to the max tc specified.
		int enl = l;
		if (!tc.no_enlightened() && tc.get_enlightened() < enl)
			enl = tc.get_enlightened();

		int res = l;
		if (!tc.no_resistance() && tc.get_resistance() < enl)
			res = tc.get_resistance();

		int neu = l;
		if (!tc.no_neutral() && tc.get_neutral() < enl)
			neu = tc.get_neutral();

		team_count layertc(enl,res,neu);

		if (sep) {
			dt.erase();
			cout << l << ": " << endl;
		}
		if (autocolour)
		{
			std::stringstream stream;
			stream << std::hex << hexcolour;
			colour = "#" + stream.str();
		}
		dt = ss.make_layer(dt,layertc,colour);

		if (autocolour)
			hexcolour -= 1118481 ;

		if (sep)
				cout << dt.to_string() << endl << endl;
		
		
	}

	if (!sep)
		cout << dt.to_string() << endl;


}
