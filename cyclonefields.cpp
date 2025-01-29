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

class cyclonefields {

private:
	unordered_map<field,int> mucache;
	draw_tools dt;
	string plan_colour;
	string edge_colour;
	run_timer rt;
	int calculation_type;
	vector<field> all;
	int sameSize;
	bool splits;
	bool edge_path;

	int cached_mu (field f);
	string draw_fields(const vector<field>& f, const vector<line>& l);
	int iterate_search(int start, line medge, vector<field>fields_list, int max, point third_point);
	int count_links (point p, vector<field> flist);
	vector<field> get_cadence(field outer, line edge, int start);
	line get_edge(vector<field>plan, field newfield);
	int next_cadence(int i, line newedge, vector<field>fields_list, field newfield, int max, vector<line>cyc_edges);


public:
	cyclonefields(draw_tools dts, draw_tools edt, run_timer rtm, int calc, bool s, bool e, const vector<field> a);
	void search_fields();

};

int cyclonefields::count_links (point p, vector<field> flist)
{

	int count = 0;
	for (field fi: flist)
		if (fi.has_point(p))
			count++;
				
	return count;
}

line cyclonefields::get_edge(vector<field>plan, field newfield)
{
	// determine non shared edges

	int count_point = -1;
	line other;
	for (line l : newfield.get_lines())
	{
		bool shared = false;
		for (field f : plan)
			if (f.has_line(l))
			{
				shared = true;
				break;
			}
		if (!shared)
		{
			// we have 1 of 2 edges, which one is it?
			int count = count_links(l.get_d_point(), plan) + count_links(l.get_o_point(), plan);
			if (count_point == -1)
			{
				count_point = count;
				other = l;
			}
			// these shouldn't evaluate to true on the first pass
			if (count < count_point)
				return l;
			if (count_point < count)
				return other;
		}
	}
	// we would only get here if count_point == count
	// which should not happen with the types of plans we are working with
	// but need some return value
	return newfield.line_at(0);
}

vector<field> cyclonefields::get_cadence (field outer, line edge, int start)
{
	vector<field> cad_fields;
	for (int j = start; j < all.size(); j++) {
		field test_field = all[j];
		// as we are limiting the continuing search space, we have to assume that the fields are sorted and decreasing in size.
		if (test_field.has_line(edge) && !outer.intersects(test_field) && outer.inside(test_field) && !(test_field == outer)) {
			cad_fields.push_back(test_field);
		}
	}
	return cad_fields;
}

int cyclonefields::next_cadence(int i, line newedge, vector<field>fields_list, field newfield, int max, vector<line>cyc_edges)
{
	fields_list.push_back(newfield);
	cyc_edges.push_back(newedge);

	if (fields_list.size() > max) {
		max = fields_list.size();
		double dispSize = 0.0;
		for (field f: fields_list)
			if (calculation_type == 0)
				dispSize += f.geo_area();
			else 
				dispSize += cached_mu(f);
		// Draw tools
		cerr << max << " : " << dispSize << " : " << rt.split() << " seconds." << endl;
		cout << draw_fields(fields_list,cyc_edges) << endl;
		cerr << endl;
	}

	vector<field> inner_cad = get_cadence(newfield, newedge, i);
	for (field cf2 : inner_cad)
	{		
		line newedge = get_edge(fields_list,cf2);
		max = next_cadence(i, newedge, fields_list, cf2, max,cyc_edges);
	}
	return max;
}

void cyclonefields::search_fields()
{
	int max = 1;
	for (int i = 0; i < all.size(); i++) {
		field this_field = all[i];
		vector<line> edges = this_field.get_lines();
		for (line edge : edges) {
			vector<field> cad_fields = get_cadence(this_field, edge, i+1);

			for (field cfi : cad_fields) {
				vector<line> medges = cfi.get_lines();
				for (line medge : medges) {
					if (!(medge == edge)) {

						vector<field> inner_cad = get_cadence(cfi, medge, i+1);

						for (field cf2 : inner_cad)
						{
							vector<field>fields_list;
							vector<line>cyclone_edges;

							// we need 3 fields to define the cyclone cadence
							fields_list.push_back(this_field);
							fields_list.push_back(cfi);

							// we should be able to implicitely determine the cadence 
							// and therefore the next edge
							line newedge = get_edge(fields_list,cf2);
							cyclone_edges.erase(cyclone_edges.begin(),cyclone_edges.end());
							cyclone_edges.push_back(edge);
							cyclone_edges.push_back(medge);
							cyclone_edges.push_back(newedge);

							max = next_cadence(i+1, newedge, fields_list, cf2, max,cyclone_edges);

						}
					}
				}
			}
		}
	}	
}


cyclonefields::cyclonefields(draw_tools dts, draw_tools edt, run_timer rtm, int calc, bool s, bool e, const vector<field> a)
{
	dt = dts;
	rt = rtm;
	calculation_type = calc;
	splits = s;
	edge_path = e;
	plan_colour = dts.get_colour();
	edge_colour = edt.get_colour();
	// this can be a shallow copy.  is this right?
	all = a;
}

int cyclonefields::cached_mu (field f)
{
	if (mucache.count(f))
		return mucache[f];

	mucache[f] = field_factory::get_instance()->get_est_mu(f);
	return mucache[f];
}

string cyclonefields::draw_fields(const vector<field>& f, const vector<line>& l)
{

	dt.erase();

	if (splits)
	{
		field_factory* ff = field_factory::get_instance();
		vector<field> sf = ff->add_splits(f);
		for (field fi: sf)
			dt.add(fi);
	} else {
		for (field fi: f)
		dt.add(fi);
	}
	if (edge_path)
	{
		dt.convert();
		dt.set_colour(edge_colour);
		for (line ll : l)
			dt.add(ll);
		dt.set_colour(plan_colour);
	}

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
		cerr << "layerlinker [options] <portal cluster> [<portal cluster> [<portal cluster>]]" << endl;
		cerr << "    if two clusters are specified, 2 portals are chosen to make links in the first cluster." << endl;
		cerr << "Options:" << endl;
		cerr << " -E <number>       Limit number of Enlightened Blockers" << endl;
		cerr << " -R <number>       Limit number of Resistance Blockers" << endl;
		cerr << " -N <number>       Limit number of Machina Blockers" << endl;
		cerr << " -s                Add split fields" << endl;
		cerr << " -C <#colour>      Set Drawtools output colour" << endl;
		cerr << " -e                Draw cyclone edge" << endl;
		cerr << " -c <#colour>      Cyclone edge path  colour" << endl;
		cerr << " -L                Set Drawtools to output as polylines" << endl;
		cerr << " -O                Output as Intel Link" << endl;
		cerr << " -T <lat,lng,...>  Use only fields covering target points" << endl;
}

int main (int argc, char* argv[])
{
	run_timer rt;

	vector<point>target;
	int calc = 0;  // area or mu
	int same_size = 0; // same calc
	bool drawedge = false;

	arguments ag(argc,argv);

	ag.add_req("E","enlightened",true); // max enlightened blockers
	ag.add_req("R","resistance",true); // max resistance blockers
	ag.add_req("N","machina",true); // max machina blockers
	
	ag.add_req("e","edges",false); // show cyclone edge path
	ag.add_req("c","edgecolour",true); // cyclone edge path colour
	ag.add_req("C","colour",true); // drawtools colour
	ag.add_req("O","polylines",false); // output as polylines
	ag.add_req("L","intel",false); // output as intel
	ag.add_req("M","MU",false); // calculate as MU
	ag.add_req("T","target",true); // target fields over location
	ag.add_req("h","help",false);
	ag.add_req("s","splits",false);

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

	if (ag.has_option("e"))
		drawedge = true;

	edt.set_colour("#5db53d");
	if (ag.has_option("c"))
	{
		if(drawedge)
			edt.set_colour(ag.get_option_for_key("c"));
		else
			cerr << "Warning: setting edge colour but not drawing edges." << endl;
	}

	if (ag.has_option("L"))
		dt.set_output_as_polyline();
	if (ag.has_option("O"))
		dt.set_output_as_intel();

	if (ag.has_option("M"))
		calc = 1;

	bool enable_splits = false;
	if (ag.has_option("s"))
		enable_splits = true;

	portal_factory* pf = portal_factory::get_instance();
	link_factory* lf = link_factory::get_instance();
	field_factory* ff = field_factory::get_instance();

	if (ag.has_option("T"))
		target = pf->points_from_string(ag.get_option_for_key("T"));

	cerr << "== Reading links and portals ==" << endl;
	rt.start();

// of course I had to pick a colliding name for my class
	vector<silicontrip::link> links;
	vector<field> all_fields;
	vector<field> af;

	try {
	if (ag.argument_size() == 1)
	{
		vector<portal> portals;
		
		portals = pf->vector_from_map(pf->cluster_from_description(ag.get_argument_at(0)));
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

	if (target.size()>0)
	{
		all_fields = ff->over_target(all_fields,target);
	}

	cerr << "==  fields filtered " << rt.split() << " seconds ==" << endl;
	cerr << "== sorting fields ==" << endl;

	sort(all_fields.begin(),all_fields.end(),geo_comparison);

	cerr << "==  fields sortered " << rt.split() << " ==" << endl;
	cerr << "== show matches ==" << endl;

	vector<pair<double,string>> plan;
	int bestbest = 0;

//	list<field> field_list;

//	for (field f: all_fields)
//		field_list.push_back(f);

	vector<field> search;
		//search.push_back(tfi);
	cyclonefields mf = cyclonefields(dt,edt,rt,calc,enable_splits,drawedge,all_fields);
	mf.search_fields();
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
