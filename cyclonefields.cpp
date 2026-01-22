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

	double get_value (vector<field> fd);
	string draw_fields(const vector<field>& f, const vector<line>& l);
	int iterate_search(int start, line medge, vector<field>fields_list, int max, point third_point);
	int count_links (const point& p, const vector<field>& flist);
	vector<field> get_cadence(const field& outer, const line& edge, int start);
	line get_edge(const vector<field>&plan, const field& newfield);
	int next_cadence(int i, const line& newedge, vector<field>&fields_list, const field& newfield, int max, vector<line>&cyc_edges);


public:
	cyclonefields(draw_tools dts, draw_tools edt, run_timer rtm, int calc, bool s, bool e, const vector<field>& a);
	void search_fields();

};

int cyclonefields::count_links (const point& p, const vector<field>& flist)
{

	int count = 0;
	for (field fi: flist)
		if (fi.has_point(p))
			count++;

	return count;
}

line cyclonefields::get_edge(const vector<field>&plan, const field& newfield)
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

vector<field> cyclonefields::get_cadence (const field& outer, const line& edge, int start)
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

int cyclonefields::next_cadence(int i, const line& newedge, vector<field>&fields_list, const field& newfield, int max, vector<line>&cyc_edges)
{
	//fields_list.push_back(newfield);
	//cyc_edges.push_back(newedge);

	if (fields_list.size() > max) {
		max = fields_list.size();
		double dispSize = get_value(fields_list);

		// Draw tools
		cerr << max << " : " << dispSize << " : " << rt.split() << " seconds." << endl;
		cout << draw_fields(fields_list,cyc_edges) << endl;
		cerr << endl;
	}

	vector<field> inner_cad = get_cadence(newfield, newedge, i);
	for (field cf2 : inner_cad)
	{
		line newedge = get_edge(fields_list,cf2);
		fields_list.push_back(cf2);
		cyc_edges.push_back(newedge);
		max = next_cadence(i, newedge, fields_list, cf2, max,cyc_edges);
		cyc_edges.pop_back();
		fields_list.pop_back();
	}
	return max;
}

void cyclonefields::search_fields()
{
	int max = 1;
	vector<field>fields_list;
	vector<line>cyclone_edges;

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
							// we need 3 fields to define the cyclone cadence
							fields_list.push_back(this_field);
							fields_list.push_back(cfi);
							fields_list.push_back(cf2);

							// we should be able to implicitely determine the cadence
							// and therefore the next edge
							line newedge = get_edge(fields_list,cf2);
							cyclone_edges.erase(cyclone_edges.begin(),cyclone_edges.end());
							cyclone_edges.push_back(edge);
							cyclone_edges.push_back(medge);
							cyclone_edges.push_back(newedge);

							max = next_cadence(i+1, newedge, fields_list, cf2, max,cyclone_edges);

							cyclone_edges.pop_back();
							cyclone_edges.pop_back();
							cyclone_edges.pop_back();

                            fields_list.pop_back();
                            fields_list.pop_back();
                            fields_list.pop_back();
						}
					}
				}
			}
		}
	}
}


cyclonefields::cyclonefields(draw_tools dts, draw_tools edt, run_timer rtm, int calc, bool s, bool e, const vector<field>& a)
{
	dt = dts;
	rt = rtm;
	calculation_type = calc;
	splits = s;
	edge_path = e;
	plan_colour = dts.get_colour();
	edge_colour = edt.get_colour();
	// this can be a shallow copy.  is this right?
	// no, I need an & in the parameter definition. I hope.
	all = a;
}

double cyclonefields::get_value (vector<field> fd)
{
	field_factory* ff = field_factory::get_instance();
	double total = 0;
	if (splits)
	{
		fd = ff->add_splits(fd);
	}
	for (field f : fd)
		if (calculation_type == 0)
			total += f.geo_area();
		else
			total += ff->get_cache_mu(f);

	return total;
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

vector<portal> cluster_and_filter_from_description(const vector<portal>& remove, const string desc)
{
	portal_factory* pf = portal_factory::get_instance();
    vector<portal> portals = pf->cluster_from_description(desc);
    if (remove.size() > 0)
        portals = pf->remove_portals(portals, remove);
    return portals;
}

vector<line> filter_lines (const vector<line>& li, const vector<silicontrip::link>& links, const team_count& tc, const vector<portal>& avoid_double, bool limit)
{
	link_factory* lf = link_factory::get_instance();
    vector<line> la = lf->filter_links(li, links, tc);
	if (avoid_double.size() > 0)
		la = lf->filter_link_by_blocker(la,links,avoid_double);
    if (limit)
        la = lf->filter_link_by_length(la, 2);

    return la;
}

void print_usage()
{
	cerr << "Usage:" << endl;
	cerr << "cyclonefields [options] <portal cluster> [<portal cluster> [<portal cluster>]]" << endl;
	cerr << "    if two clusters are specified, 2 portals are chosen to make links in the first cluster." << endl;
	cerr << "Options:" << endl;
	cerr << " -E <number>       Limit number of Enlightened Blockers" << endl;
	cerr << " -R <number>       Limit number of Resistance Blockers" << endl;
	cerr << " -N <number>       Limit number of Machina Blockers" << endl;
	cerr << " -D <cluster>      Filter links crossing blockers using these portals." << endl;
	cerr << " -S <cluster>      Avoid linking to these portals" << endl;

	cerr << " -s                Add split fields" << endl;
	cerr << " -M                Use MU calculation" << endl;
	cerr << " -C <#colour>      Set Drawtools output colour" << endl;
	cerr << " -e                Draw cyclone edge" << endl;
	cerr << " -c <#colour>      Cyclone edge path  colour" << endl;
	cerr << " -L                Set Drawtools to output as polylines" << endl;
	cerr << " -I                Output as Intel Link" << endl;
	cerr << " -k                Limit links to 2km" << endl;
	cerr << " -T <lat,lng,...>  Use only fields covering target points" << endl;
}

int main (int argc, char* argv[])
{
	run_timer rt;

	vector<point>target;
	int calc = 0;  // area or mu
	int same_size = 0; // same calc
	bool drawedge = false;
	bool limit = false;
	vector<portal>avoid_double;
	vector<portal>avoid_single;

	arguments ag(argc,argv);

	ag.add_req("E","enlightened",true); // max enlightened blockers
	ag.add_req("R","resistance",true); // max resistance blockers
	ag.add_req("N","machina",true); // max machina blockers
	ag.add_req("D","blockers",true); // remove links with blocker using these portals.
	ag.add_req("S","avoid", true); // avoid using these portals.


	ag.add_req("e","edges",false); // show cyclone edge path
	ag.add_req("c","edgecolour",true); // cyclone edge path colour
	ag.add_req("C","colour",true); // drawtools colour
	ag.add_req("I","intel",false); // output as intel
	ag.add_req("L","polyline",false); // output as polylines
	ag.add_req("M","MU",false); // calculate as MU
	ag.add_req("T","target",true); // target fields over location
	ag.add_req("h","help",false);
	ag.add_req("s","splits",false);
	ag.add_req("k","limit2k",false);

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
	if (ag.has_option("I"))
		dt.set_output_as_intel();

	if (ag.has_option("M"))
		calc = 1;

	if (ag.has_option("k"))
		limit = true;

	bool enable_splits = false;
	if (ag.has_option("s"))
		enable_splits = true;

	portal_factory* pf = portal_factory::get_instance();
	link_factory* lf = link_factory::get_instance();
	field_factory* ff = field_factory::get_instance();

	if (ag.has_option("T"))
		target = pf->points_from_string(ag.get_option_for_key("T"));

	if (ag.has_option("D"))
		avoid_double = pf->cluster_from_description(ag.get_option_for_key("D"));

	if (ag.has_option("S"))
		avoid_single = pf->cluster_from_description(ag.get_option_for_key("S"));

	cerr << "== Reading links and portals ==" << endl;
	rt.start();

	vector<silicontrip::link> links;
	vector<field> all_fields;
	vector<field> af;
	vector<vector<portal>> clusters;
	vector<portal> all_portals;

	try {
		if (ag.argument_size() == 1) {
			clusters.push_back(cluster_and_filter_from_description(avoid_single, ag.get_argument_at(0)));
		} else if (ag.argument_size() == 2) {
			clusters.push_back(cluster_and_filter_from_description(avoid_single, ag.get_argument_at(0)));
			clusters.push_back(cluster_and_filter_from_description(avoid_single, ag.get_argument_at(1)));
		} else if (ag.argument_size() == 3) {
			clusters.push_back(cluster_and_filter_from_description(avoid_single, ag.get_argument_at(0)));
			clusters.push_back(cluster_and_filter_from_description(avoid_single, ag.get_argument_at(1)));
			clusters.push_back(cluster_and_filter_from_description(avoid_single, ag.get_argument_at(2)));
		} else {
			print_usage();
			exit(1);
		}

		for (const vector<portal>& cluster : clusters) {
			all_portals.insert(all_portals.end(), cluster.begin(), cluster.end());
		}

		cerr << "== " << all_portals.size() << " portals read. in " << rt.split() << " seconds. ==" << endl;
		cerr << "== getting links ==" << endl;

		links = lf->get_purged_links(all_portals);

		cerr <<  "== " << links.size() << " links read. in " << rt.split() <<  " seconds ==" << endl;
		cerr << "== generating potential links ==" << endl;

		if (ag.argument_size() == 1) {
			vector<line> li = lf->make_lines_from_single_cluster(clusters[0]);
			cerr << "all links: " << li.size() << endl;

			li = filter_lines(li, links, tc, avoid_double, limit);

			cerr << "purged links: " << li.size() << endl;
			cerr << "== " << li.size() << " links generated " << rt.split() << " seconds. Generating fields ==" << endl;

			af = ff->make_fields_from_single_links(li);
		} else if (ag.argument_size() == 2) {
			vector<line> li1 = filter_lines(lf->make_lines_from_single_cluster(clusters[0]), links, tc, avoid_double, limit);
			cerr << "== cluster 1 links:  " << li1.size() << " ==" << endl;

			vector<line> li2 = filter_lines(lf->make_lines_from_double_cluster(clusters[0], clusters[1]), links, tc, avoid_double, limit);
			cerr << "== cluster 2 links:  " << li2.size() << " ==" << endl;

			af = ff->make_fields_from_double_links(li2, li1);
		} else if (ag.argument_size() == 3) {
			vector<line> li1 = filter_lines(lf->make_lines_from_double_cluster(clusters[0], clusters[1]), links, tc, avoid_double, limit);
			cerr << "== cluster 1 links:  " << li1.size() << " ==" << endl;

			vector<line> li2 = filter_lines(lf->make_lines_from_double_cluster(clusters[1], clusters[2]), links, tc, avoid_double, limit);
			cerr << "== cluster 2 links:  " << li2.size() << " ==" << endl;

			vector<line> li3 = filter_lines(lf->make_lines_from_double_cluster(clusters[2], clusters[0]), links, tc, avoid_double, limit);
			cerr << "== cluster 3 links:  " << li3.size() << " ==" << endl;

			af = ff->make_fields_from_triple_links(li1, li2, li3);
		}

		all_fields = ff->filter_fields(af,links,tc);
		cerr << "fields: " << all_fields.size() << endl;
		cerr << "==  fields generated " << rt.split() << " seconds ==" << endl;

	if (target.size()>0)
	{
		all_fields = ff->over_target(all_fields,target);
	}

	cerr << "==  fields filtered " << rt.split() << " seconds ==" << endl;

	if (all_fields.size() == 0)
	{
		cerr << "No fields remaining after filtering" << endl;
		exit(1);
	}
	cerr << "== sorting fields ==" << endl;

	sort(all_fields.begin(),all_fields.end(),geo_comparison);

	cerr << "==  fields sortered " << rt.split() << " ==" << endl;
	cerr << "== show matches ==" << endl;

	vector<pair<double,string>> plan;
	int bestbest = 0;

	vector<field> search;
	//search.push_back(tfi);
	cyclonefields mf = cyclonefields(dt,edt,rt,calc,enable_splits,drawedge,all_fields);
	mf.search_fields();
	//search_fields(dt,search,all_fields,0,0,calc,same_size,0.0,rt);

	cerr << "==  plans searched " << rt.split() << " seconds ==" << endl;
	cerr <<  "== Finished. " << rt.split() << " elapsed time. " << rt.stop() << " total time." << endl;

	} catch (exception &e) {
		cerr << "An Error occured: " << e.what() << endl;
	}

	return 0;
}
