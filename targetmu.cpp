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
#include "uniform_distribution.hpp"

using namespace std;
using namespace silicontrip;



class targetmu {

private:
	unordered_map<field,int> mucache;
	draw_tools dt;
	run_timer rt;
	int calculation_type;
	vector<field> all;
	int sameSize;
	long target_mu;
	int max_fields_limit;

	string draw_fields(const vector<field>& f);
	uniform_distribution get_value (vector<field> fd);

public:
	targetmu(draw_tools dts, run_timer rtm, const vector<field>& a, long tm = -1, int field_limit = 0);
	int search_fields(vector<field>& current, int start, int max);

};

targetmu::targetmu(draw_tools dts, run_timer rtm, const vector<field>& a, long tm, int field_limit)
{
	dt = dts;
	rt = rtm;
	// this can be a shallow copy.  is this right?
	all = a;
	target_mu = tm;
	max_fields_limit = field_limit;
}

string targetmu::draw_fields(const vector<field>& f)
{
	dt.erase();

	for (field fi: f)
		dt.add(fi);

	return dt.to_string();
}

uniform_distribution targetmu::get_value (vector<field> fd)
{
	field_factory* ff = field_factory::get_instance();
	uniform_distribution total(0,0);

	for (field f : fd)
		total += ff->get_cache_ud_mu(f).mu_rounded();

	return total;
}

int targetmu::search_fields(vector<field>& current, int start, int max)
{
	

	if (current.size() > 0)
	{
		uniform_distribution current_mu = get_value(current);
		
		int score = abs(round(current_mu.get_lower()) - target_mu) + abs (round(current_mu.get_upper()) - target_mu);

		if (score < max)
		{
			max = score;
			cerr << " " << max << ": MU: " << current_mu << " Fields: " << current.size() << " : "  << rt.split() << " seconds." << endl;
			cout << draw_fields(current) << endl;
			cerr << endl;
		}
	}
	if (max_fields_limit > 0 && current.size() >= max_fields_limit) {
		return max;
	}
	for (int i=start; i<all.size(); i++)
	{
		field thisField = all.at(i);
		if (!thisField.intersects(current))
		{
			current.push_back(thisField);
			int score = search_fields(current, i+1, max);
			current.pop_back();
			if (score < max)
				max = score;
		}
	}
	return max;
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

vector<line> filter_lines (const vector<line>& li, const vector<silicontrip::link>& links, const team_count& tc, const vector<portal>& avoid_double, bool limit2k, double percentile)
{
	link_factory* lf = link_factory::get_instance();
    vector<line> la = lf->filter_links(li, links, tc);
    if (avoid_double.size() > 0)
        la = lf->filter_link_by_blocker(la, links, avoid_double);

    if (limit2k)
        la = lf->filter_link_by_length(la, 2);

    if (percentile < 100)
        la = lf->percentile_lines(la, percentile);

    return la;
}

void print_usage()
{
		cerr << "Usage:" << endl;
		cerr << "targetmu [options] -x mu <portal cluster> [<portal cluster> [<portal cluster>]]" << endl;
		cerr << "    if two clusters are specified, 2 portals are chosen to make links in the first cluster." << endl;
		cerr << "Generates the maximum number of fields possible for a given portal cluster descriptions." << endl;
		cerr << "Options:" << endl;
		cerr << " -E <number>       Limit number of Enlightened Blockers" << endl;
		cerr << " -R <number>       Limit number of Resistance Blockers" << endl;
		cerr << " -N <number>       Limit number of Machina Blockers" << endl;
		cerr << " -D <cluster>      Filter links crossing blockers using these portals" << endl;
		cerr << " -a <cluster>      Avoid linking to these portals" << endl;
		cerr << " -i <cluster>      Ignore blocking links from these portals" << endl;
		cerr << " -k                Limit links to 2km" << endl;


		cerr << " -C <#colour>      Set Drawtools output colour" << endl;
		cerr << " -L                Set Drawtools to output as polylines" << endl;
		cerr << " -I                Output as Intel Link" << endl;
		cerr << " -x <MU>           Target exactly <MU> amount" << endl;
		cerr << " -l <number>       Limit maximum number of fields" << endl;
		cerr << " -T <lat,lng,...>  Use only fields covering target points" << endl;
}

int main (int argc, char* argv[])
{
	run_timer rt;

	vector<point>target;
	vector<portal>avoid_double;
	vector<portal>avoid_single;
	vector<portal>ignore_links;
	bool limit2k = false;
	double percentile = 100;
	long target_mu = -1;
	int max_fields = 0;

	arguments ag(argc,argv);

	ag.add_req("E","enlightened",true); // max enlightened blockers
	ag.add_req("R","resistance",true); // max resistance blockers
	ag.add_req("N","machina",true); // max machina blockers
	ag.add_req("i","ignore",true); // ignore links from these portals (about to decay or easy to destroy)
	ag.add_req("a","avoid", true); // avoid using these portals.  S in other tools
	ag.add_req("k","limit2k",false); // limit link length to that can be made under fields.
	ag.add_req("p","lpercent",true); // use percentile longest links
	ag.add_req("D","blockers",true); // remove links with blocker using these portals.


	ag.add_req("C","colour",true); // drawtools colour
	ag.add_req("I","intel",false); // output as intel
	ag.add_req("L","polyline",false); // output as polylines
	ag.add_req("x","targetmu",true); // Target MU
	ag.add_req("l","limit",true); // Limit fields

	ag.add_req("T","target",true); // target fields over location
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

	if (ag.has_option("C")) {
		cerr << "set colour: " << ag.get_option_for_key("C") << endl;
		dt.set_colour(ag.get_option_for_key("C"));
	}

	if (ag.has_option("L"))
		dt.set_output_as_polyline();
	if (ag.has_option("I"))
		dt.set_output_as_intel();

	if (ag.has_option("k"))
		limit2k=true;

	if (ag.has_option("p"))
		percentile = ag.get_option_for_key_as_double("p");

	if (ag.has_option("x")) {
		target_mu = ag.get_option_for_key_as_int("x"); // Assuming int handles long enough for MU? 
		// get_option_for_key_as_int returns int. MU could be large. 
		// I should check arguments.hpp if there is as_long or just use atol(get_option_for_key).
	}

	if (target_mu == -1) {
		cerr << "No target MU specified" << endl;
		exit(1);
	}

	if (ag.has_option("l"))
		max_fields = ag.get_option_for_key_as_int("l");

	portal_factory* pf = portal_factory::get_instance();
	link_factory* lf = link_factory::get_instance();
	field_factory* ff = field_factory::get_instance();

	if (ag.has_option("T"))
		target = pf->points_from_string(ag.get_option_for_key("T"));

	if (ag.has_option("D"))
		avoid_double = pf->cluster_from_description(ag.get_option_for_key("D"));

	if (ag.has_option("a"))
		avoid_single = pf->cluster_from_description(ag.get_option_for_key("a"));

	cerr << "== Reading links and portals ==" << endl;
	rt.start();

// of course I had to pick a colliding name for my class
	vector<silicontrip::link> links;
	vector<field> all_fields;
	vector<vector<portal>> clusters;
	vector<portal> all_portals;

	//vector<field> af;

	try {if (ag.argument_size() == 1) {
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

	    // Get purged links
    vector<silicontrip::link> links = lf->get_purged_links(all_portals);
	if (!ignore_links.empty())
		links = lf->filter_link_by_portal(links,ignore_links);
    cerr <<  "== " << links.size() << " links read. in " << rt.split() <<  " seconds ==" << endl;
    cerr << "== generating potential links ==" << endl;

	if (ag.argument_size() == 1) {
        vector<line> li = lf->make_lines_from_single_cluster(clusters[0]);
        cerr << "all links: " << li.size() << endl;

        li = filter_lines(li, links, tc, avoid_double, limit2k, percentile);

        cerr << "== " << li.size() << " links generated " << rt.split() << " seconds. Generating fields ==" << endl;

        all_fields = ff->make_fields_from_single_links(li);
    } else if (ag.argument_size() == 2) {
        vector<line> li1 = filter_lines(lf->make_lines_from_single_cluster(clusters[0]), links, tc, avoid_double, limit2k, percentile);
        cerr << "== cluster 1 links:  " << li1.size() << " ==" << endl;

        vector<line> li2 = filter_lines(lf->make_lines_from_double_cluster(clusters[0], clusters[1]), links, tc, avoid_double, limit2k, percentile);
        cerr << "== cluster 2 links:  " << li2.size() << " ==" << endl;

        all_fields = ff->make_fields_from_double_links(li2, li1);
    } else if (ag.argument_size() == 3) {
        vector<line> li1 = filter_lines(lf->make_lines_from_double_cluster(clusters[0], clusters[1]), links, tc, avoid_double, limit2k, percentile);
        cerr << "== cluster 1 links:  " << li1.size() << " ==" << endl;

        vector<line> li2 = filter_lines(lf->make_lines_from_double_cluster(clusters[1], clusters[2]), links, tc, avoid_double, limit2k, percentile);
        cerr << "== cluster 2 links:  " << li2.size() << " ==" << endl;

        vector<line> li3 = filter_lines(lf->make_lines_from_double_cluster(clusters[2], clusters[0]), links, tc, avoid_double, limit2k, percentile);
        cerr << "== cluster 3 links:  " << li3.size() << " ==" << endl;

        all_fields = ff->make_fields_from_triple_links(li1, li2, li3);
    }

	cerr << "== " << all_fields.size() << " fields generated " << rt.split() << " seconds ==" << endl;

    // Common field processing

	if (ag.has_option("E") || ag.has_option("R") || ag.has_option("N"))
	    all_fields = ff->filter_existing_fields(all_fields, links); // need to make this optional. Or related to team blockers.

    all_fields = ff->filter_fields(all_fields, links, tc);

	if (!target.empty())
	{
		all_fields = ff->over_target(all_fields,target);
	}

	cerr << "== " << all_fields.size() << " fields filtered " << rt.split() << " seconds ==" << endl;
	if (all_fields.size() == 0)
	{
		cerr << "No fields remaining after filtering" << endl;
		exit(1);
	}
	cerr << "== sorting fields ==" << endl;

	sort(all_fields.begin(),all_fields.end(),geo_comparison);

	cerr << "==  fields sortered " << rt.split() << " ==" << endl;
	cerr << "== show matches ==" << endl;

	/*
	// remove once duplicate bug is found
	for (int i=0; i<all_fields.size(); i++)
		for (int j=i+1; j < all_fields.size(); j++)
			if (all_fields[i] == all_fields[j])
				cerr << "Duplicate: " << all_fields[i] << endl;
	*/



	vector<field> search;
		//search.push_back(tfi);
	targetmu tm = targetmu(dt,rt,all_fields,target_mu, max_fields);
	//field fi = all_fields[0];
	//search.push_back(fi);
	
	// If target_mu is set, we need to initialize 'max' (which becomes min_diff) to a large value.
	int initial_max = 0;

	initial_max = 2000000000; // 2 billion should be enough for diff?
	

	// Start search with empty list, index 0.
	int result = tm.search_fields(search, 0, initial_max);
	//search.pop_back();
	//search_fields(dt,search,all_fields,0,0,calc,same_size,0.0,rt);

	cerr << "==  plans searched " << rt.split() << " seconds ==" << endl;


	cerr <<  "== Finished. " << rt.split() << " elapsed time. " << rt.stop() << " total time." << endl;

	} catch (exception &e) {
		cerr << "An Error occured: " << e.what() << endl;
	}

	return 0;
}
