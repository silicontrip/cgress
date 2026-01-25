#include <iostream>
#include <chrono>
#include <thread>
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

struct score {
	int count;
	double balance;
};

class exofields {

private:
	unordered_map<field,int> mucache;
	draw_tools dt;

	run_timer rt;
	vector<field> all;
	vector<portal> all_portals;
	int limit;
	int minimum;
	bool butterfly;

	int cached_mu (field f);
	string draw_fields(const vector<field>& f, const vector<line>& l, const vector<point>& p);
	int exo_search (point po, int max);
	vector<line> generate_spine(const vector<field>& fa);
	vector<point> exo_spine_search (point po, const vector<field>& pf, line shared, const vector<field>& spine);
	int next_search (int start, field nf, vector<field> pf, line sh, vector<field> sp, point po, int max);
	vector<field> find_fields_with_portal(vector<field> fa, point po);
	vector<field> find_fields_with_link(vector<field> fa, line li);


public:
	exofields(draw_tools dts, run_timer rtm, int lim, int min, const vector<field>& a, const vector<portal>& ap);
	void search_fields(const vector<portal>& p, int m);

};

// takes a vector of nested fields, all sharing a common base link and generates the splits spine
vector<line> exofields::generate_spine(const vector<field>& fa)
{
	//cerr << " >>> generate_spine" << endl;

	int cs = fa.size();
	vector<field> fa_ordered(cs);

	vector<line> la(cs-1);
	line shared = fa[0].shared_line(fa[1]);

	//cerr << "Shared: " << shared << endl;

	for (field f1: fa)
	{
		//cerr << "Field: " << f1 << endl;
		int count = 0;
		for (field f2: fa)
			if (f2.inside(f1))
				count++;
		//	System.out.println("Count: " + count);
		//cerr << "Count: " << count << endl;
		fa_ordered[count-1]=f1;
	}
		
	point old_point;
	bool first = true;
	int index = 0;
	for (field f1: fa_ordered)
	{
		point p = f1.other_point(shared);
		//cerr << "point: " << p << endl;
		if (!first)
		{
			line l = line(old_point,p);
			la[index++]=l;
		}
		old_point=p;
		first = false;
	}
	//cerr << " <<< generate_spine" << endl;

	return la;
}

// this finds potential throwing portals
// so should return an array (vector) of possible points.
vector<point> exofields::exo_spine_search (point po, const vector<field>& pf, line shared, const vector<field>& spine)
{

	//vector<vector<field>> solution;
	vector<line> spine_links = generate_spine(spine);
	vector<field> fan_fields;
	vector<point> throw_points;

	for (line l: spine_links)
		fan_fields.push_back(field(po,l.get_o_point(), l.get_d_point()));

	vector<point> spine_points;
	for (field f: spine)
		spine_points.push_back(f.other_point(shared));

	vector<point> anchors; 
	if (po == shared.get_d_point())
		anchors.push_back(shared.get_o_point());
	else 
		anchors.push_back(shared.get_d_point());

	point anchor;

	//cerr << "  exo_spine_search::for field fi" << endl;

	for (field fi: pf)
	{
		// find a field which isn't part of this spine.
		// field does not have shared

		if (!fi.has_line(shared)) {
			bool ok=false;
			for (point pfi: spine_points)
			{
				line fan_line(po,pfi);
				// fields are not the same
				// and they share a link
				anchor = fi.other_point(fan_line);
				bool used = false;
				for (point p: anchors)
					if (p == anchor)
					{
						used=true;
						break;
					}
				// don't use a spine portal (not sure why it's picking them up)
				for (point p: spine_points)
					if (p==anchor)
					{
						used=true;
						break;
					}

				bool has_line = fi.has_line(fan_line);
				bool layers = fi.layers(spine); 
				bool intersects = fi.intersects(spine_links);

				if ( has_line && layers && !intersects && !used )
				{
					//dt.erase(); dt.addField(fi); System.out.println("Field matches spine: " + dt);
					ok=true;
					break;
				}
			}
			//cerr << "  exo_spine_search::if ok" << endl;

			if (ok) {
				// this field works with one of the spine fields
				// does the target portal work with all fields?
				bool all_spine = true;
				vector<field> new_spine;
				for (field spf: spine) 
				{
					bool field_ok = false;
					for (field ifi: pf)
						if (ifi.has_point(anchor) && !(ifi==fi) && ifi.shares_line(spf) && !ifi.intersects(new_spine) && !ifi.found_in(new_spine) && !ifi.intersects(fan_fields) )
						{
							new_spine.push_back(ifi);
							field_ok = true;
							break;
						}
					if (!field_ok)
					{
						all_spine=false;	
						break;
					}
				}
				if (all_spine)
				{
					//solution.push_back(new_spine);
					anchors.push_back(anchor);
					throw_points.push_back(anchor);
				}
			}
		}
	}

	// need to remove the base anchors
	return throw_points;
}

int exofields::next_search (int start, field nf, vector<field> pf, line sh, vector<field> sp, point po, int max)
{
	sp.push_back(nf);
	if (sp.size() >= minimum)
	{
		vector<point> thr = exo_spine_search(po,pf,sh,sp);
		if (thr.size() == 0)
			return max;  // I assume that the current spine blocks out valid throw portals.
		// draw spine...
		vector<line> spine_links = generate_spine(sp);

		int possible = thr.size();
		if (thr.size() > limit)
			thr.resize(limit);

		int score_per_throw = (sp.size()-2)*2 + 1;
		int base_fields = sp.size() - 2;
		int score = base_fields + thr.size() * score_per_throw;

		// and handle butterfly fields.

		if (thr.size() > 0 && score > max && sp.size() >= minimum )
		{
			dt.erase();

			for (line li : spine_links)
			{
				field fi(po,li.get_d_point(),li.get_o_point());
				dt.add(fi);
			}
			for (point pi: thr)
				dt.add(pi);

			max = score;
			cerr << "total fields:" << score << " (" << thr.size() << "x" << score_per_throw << "+" << sp.size() -2 << ") throw points: " << thr.size() << " (" << possible << ") spine size: " << sp.size() << " : " << rt.split() << " seconds." << endl;
			cout << dt.to_string() << endl << endl;
		}
	}
	for (int k=start+1; k < pf.size(); k++)
	{
		field f3 = pf[k];
		if (f3.has_line(sh) && !f3.intersects(sp) && f3.layers(sp) )
		{						
			// find the most portals that can use this spine	
			// this depends on if we want a single fan field with multiple throws
			max = next_search(k,f3,pf,sh,sp,po,max);
		}

	}
	return max;
}

int exofields::exo_search (point po, int max)
{
	vector<field> portal_fields = find_fields_with_portal(all,po);
	vector<field> searched;

	// iterate through fields with matching links.
	for (int i=0; i< portal_fields.size(); i++)
	{
		bool space_searched = false;
		field f1 = portal_fields[i];

	// we're going to assume "similar" regions already have found the best plan.
	// look somewhere completely different
		if (f1.intersects(searched))
		{
				space_searched = true;
				//cerr << "Skipping: " << f1 << endl;
		}

		if (!space_searched)
		{
			vector<field> spine;
			spine.push_back(f1);
		

	//				dt.erase();
	//				dt.add(f1);
					//cerr << "Shared: " << shared << " : " << rt.split() << " seconds." << endl;
	//				cerr << dt.to_string() << endl << endl;

			for (int j=i+1; j< portal_fields.size(); j++)
			{
				field f2 = portal_fields[j];
				// do fields share a common link?
				if (f2.shares_line(f1) && !f2.intersects(f1) && f2.layers(f1))
				{

					//spine.push_back(f2);
					// 	this is our base link
					line shared = f1.shared_line(f2);

					// this needs to be a combination search
					int found = next_search(j,f2,portal_fields,shared,spine,po,max);
					// or multiple fan fields with a single throw
					// cerr << "score: " << found << endl;

					// only remove area if we found a plan
					if (found > 1)
						searched.push_back(f1);
					if (minimum == 0)
						max = found;
				}
			}
		}
	}

	return max;

}

vector<field> exofields::find_fields_with_portal(vector<field> fa, point po)
{
	vector<field> nl; 

	for (field fi: fa)
	{
		if (fi.has_point(po))
		{
			nl.push_back(fi);
		}
	}

	return nl;
}

vector<field> exofields::find_fields_with_link(vector<field> fa, line li)
{
	vector<field> nl;
	for (field fi: fa)
		if (fi.has_line(li))
			nl.push_back(fi);
	return nl;
}

void exofields::search_fields(const vector<portal>& p, int m)
{
	int max = 1;
	for (portal po: p) // this is the fan field originating portal. (not the throws portal)
	{
		cerr << "portal: " << po << " : " <<  rt.split() << " seconds." << endl;
		max = exo_search (po,max);
	}

}

exofields::exofields(draw_tools dts, run_timer rtm, int lim, int min, const vector<field>& a, const vector<portal>& ap)
{
	dt = dts;
	rt = rtm;
	limit = lim;
	minimum = min;

	// this can be a shallow copy.  is this right?
	all = a;
	all_portals = ap;
}

// this should move into field factory.
int exofields::cached_mu (field f)
{
	if (mucache.count(f))
		return mucache[f];

	mucache[f] = field_factory::get_instance()->get_est_mu(f);
	return mucache[f];
}

string exofields::draw_fields(const vector<field>& fan, const vector<line>& spine, const vector<point>& p)
{

	dt.erase();
	string colour = dt.get_colour();
	//dt.set_colour("#c040c0");
	for(field f: fan)
		dt.add(f);
		
	dt.set_colour("#c0c000");
	for (point anc: p)
		dt.add(anc);

	dt.set_colour(colour);
	for(line l: spine)
		dt.add(l);

	return dt.to_string();
}


bool geo_comparison(const field& a, const field& b)
{
    return a.geo_area() > b.geo_area();
}

bool pair_sort(const pair<double,string>& a, const pair<double,string>& b)
{
	return a.first < b.first;
}

void print_usage()
{
		cerr << "Usage:" << endl;
		cerr << "exofields [options] <portal cluster> [<portal cluster> [<portal cluster>]]" << endl;
		cerr << "    if two clusters are specified, 2 portals are chosen to make links in the first cluster." << endl;
		cerr << "Options:" << endl;
		cerr << " -E <number>       Limit number of Enlightened Blockers" << endl;
		cerr << " -R <number>       Limit number of Resistance Blockers" << endl;
		cerr << " -N <number>       Limit number of Machina Blockers" << endl;
		cerr << " -C <#colour>      Set Drawtools output colour" << endl;
		cerr << " -L                Set Drawtools to output as polylines" << endl;
		cerr << " -r <number>       Limit the number of rethrows" << endl;
		cerr << " -S <portal cluster> source portal cluster" << endl;
}

int main (int argc, char* argv[])
{
	run_timer rt;

	vector<point>target;
	int calc = 0;  // area or mu
	int limit=-1; // no limit
	int minimum = 0; // only print best

	arguments ag(argc,argv);

	ag.add_req("E","enlightened",true); // max enlightened blockers
	ag.add_req("R","resistance",true); // max resistance blockers
	ag.add_req("N","machina",true); // max machina blockers
	
	ag.add_req("C","colour",true); // drawtools colour
	ag.add_req("O","polylines",false); // output as polylines
	ag.add_req("M","MU",false); // calculate as MU
	ag.add_req("r","rethrows",true); // limit rethrows (flip and go to new portal)
    ag.add_req("S","source",true); // source portal cluster
	ag.add_req("m","minimum",true); // minimum number of layers
	ag.add_req("h","help",false);

	if (!ag.parse())
	{
		print_usage();
		exit(1);
	}

	if (ag.has_option("h"))
	{
		print_usage();
		exit(1);
	}

	team_count tc = team_count(ag.get_option_for_key("E"),ag.get_option_for_key("R"),ag.get_option_for_key("N"));
	draw_tools dt;
	draw_tools edt;

	if (ag.has_option("C")) {
		cerr << "set colour: " << ag.get_option_for_key("C") << endl;
		dt.set_colour(ag.get_option_for_key("C"));
	}

	if (ag.has_option("L"))
		dt.set_output_as_polyline();


	if (ag.has_option("M"))
		calc = 1;

	if (ag.has_option("r"))
		limit = ag.get_option_for_key_as_int("r");

	if (ag.has_option("m"))
		minimum = ag.get_option_for_key_as_int("m");

	portal_factory* pf = portal_factory::get_instance();
	link_factory* lf = link_factory::get_instance();
	field_factory* ff = field_factory::get_instance();

	cerr << "== Reading links and portals ==" << endl;
	rt.start();

// of course I had to pick a colliding name for my class
	vector<silicontrip::link> links;
	vector<field> all_fields;
	vector<field> af;
	vector<portal> allp;

	try {
	if (ag.argument_size() == 1)
	{
		vector<portal> portals;
		
		portals = pf->vector_from_map(pf->cluster_from_description(ag.get_argument_at(0)));
		allp.assign(portals.begin(),portals.end());

		cerr << "== " << portals.size() << " portals read. in " << rt.split() << " seconds. ==" << endl;

		cerr << "== getting links ==" << endl;
                                
		links = lf->get_purged_links(portals);
                                
		cerr <<  "== " << links.size() << " links read. in " << rt.split() <<  " seconds ==" << endl;

		cerr << "== generating potential links ==" << endl;

		vector<line> li = lf->make_lines_from_single_cluster(portals);
		cerr << "all links: " << li.size() << endl;
		li = lf->filter_links(li,links,tc);
					
		cerr << "purged links: " << li.size() << endl;
		cerr << "==  links generated " << rt.split() <<  " seconds ==" << endl;
		cerr << "== Generating fields ==" << endl;

		af = ff->make_fields_from_single_links(li);
		all_fields = ff->filter_fields(af,links,tc);
		cerr << "fields: " << all_fields.size() << endl;

	} else if (ag.argument_size() == 2) {
		// 2 portals from first cluster, 1 portal from second cluster
		vector<portal> portals1;
		vector<portal> portals2;

		portals1 = pf->vector_from_map(pf->cluster_from_description(ag.get_argument_at(0)));
		portals2 = pf->vector_from_map(pf->cluster_from_description(ag.get_argument_at(1)));

		vector<portal> all_portals;

		all_portals.insert( all_portals.end(), portals1.begin(), portals1.end() );
		all_portals.insert( all_portals.end(), portals2.begin(), portals2.end() );

		allp.assign(all_portals.begin(),all_portals.end());

		cerr << "== " << all_portals.size() << " portals read. in " << rt.split() << " seconds. ==" << endl;
		cerr << "== getting links ==" << endl;
                                
		links = lf->get_purged_links(all_portals);
                                
		cerr <<  "== " << links.size() << " links read. in " << rt.split() <<  " seconds ==" << endl;
		cerr << "== generating potential links ==" << endl;

		vector<line> li1 = lf->make_lines_from_single_cluster(portals1);
		li1 = lf->filter_links(li1,links,tc);

		cerr << "== cluster 1 links:  " << li1.size() << " ==" << endl;

		vector<line> li2 = lf->make_lines_from_double_cluster(portals1,portals2);
		li2 = lf->filter_links(li2,links,tc);	

		cerr << "== cluster 2 links:  " << li2.size() << " ==" << endl;

		all_fields = ff->make_fields_from_double_links(li2,li1);
		all_fields = ff->filter_fields(all_fields,links,tc);

		cerr << "== Fields:  " << all_fields.size() << " ==" << endl;
	
	} else if (ag.argument_size() == 3) {
		vector<portal> portals1;
		vector<portal> portals2;
		vector<portal> portals3;

		portals1 = pf->vector_from_map(pf->cluster_from_description(ag.get_argument_at(0)));
		portals2 = pf->vector_from_map(pf->cluster_from_description(ag.get_argument_at(1)));
		portals3 = pf->vector_from_map(pf->cluster_from_description(ag.get_argument_at(2)));

		vector<portal> all_portals;
		all_portals.insert(all_portals.end(), portals1.begin(), portals1.end());
		all_portals.insert(all_portals.end(), portals2.begin(), portals2.end());
		all_portals.insert(all_portals.end(), portals3.begin(), portals3.end());
		allp.assign(all_portals.begin(),all_portals.end());

		cerr << "== " << all_portals.size() << " portals read. in " << rt.split() << " seconds. ==" << endl;
		cerr << "== getting links ==" << endl;
                                
		links = lf->get_purged_links(all_portals);
                                
		cerr <<  "== " << links.size() << " links read. in " << rt.split() <<  " seconds ==" << endl;
		cerr << "== generating potential links ==" << endl;

		vector<line> li1 = lf->make_lines_from_double_cluster(portals1,portals2);
		li1 = lf->filter_links(li1,links,tc);

		cerr << "== cluster 1 links:  " << li1.size() << " ==" << endl;

		vector<line> li2 = lf->make_lines_from_double_cluster(portals2,portals3);
		li2 = lf->filter_links(li2,links,tc);

		cerr << "== cluster 2 links:  " << li2.size() << " ==" << endl;

		vector<line> li3 = lf->make_lines_from_double_cluster(portals3,portals1);
		li3 = lf->filter_links(li3,links,tc);

		cerr << "== cluster 3 links:  " << li1.size() << " ==" << endl;

		all_fields = ff->make_fields_from_triple_links(li1,li2,li3);
		all_fields = ff->filter_fields(all_fields,links,tc);

	} else {
		print_usage();
		exit(1);
	}
	cerr << "==  fields generated " << rt.split() << " seconds ==" << endl;
	cerr << "== sorting fields ==" << endl;

	sort(all_fields.begin(),all_fields.end(),geo_comparison);

	cerr << "==  fields sortered " << rt.split() << " ==" << endl;
	cerr << "== show matches ==" << endl;

	exofields mf = exofields(dt,rt,limit,minimum,all_fields,allp);
	if (ag.has_option("S"))
	{
		cerr << "Source Portals: " << ag.get_option_for_key("S") << endl;
		vector<portal> pp = pf->vector_from_map(pf->cluster_from_description(ag.get_option_for_key("S")));
		mf.search_fields(pp,0);
	} else {
		mf.search_fields(allp,0);
	}
	//search_fields(dt,search,all_fields,0,0,calc,same_size,0.0,rt);

	cerr << "==  plans searched " << rt.split() << " seconds ==" << endl;
	//cerr <<  "== show all plans ==" << endl;

	//sort (plan.begin(), plan.end(), pair_sort);
	//for (pair<double,string> entry: plan) 
	//{
	//	cout <<  entry.first << " " << entry.second << endl <<endl;
	//}

	cerr <<  "== Finished. " << rt.split() << " elapsed time. " << rt.stop() << " total time." << endl;

	} catch (exception &e) {
		cerr << "An Error occured: " << e.what() << endl;
	}

	return 0;	
}
