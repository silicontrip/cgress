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

class maxfields {

private:
	unordered_map<field,int> mucache;
	draw_tools dt;
	run_timer rt;
	int calculation_type;
	vector<field> all;
	int sameSize;

	int cached_mu (field f);
	string draw_fields(const vector<field>& f);
	double calculate_balance_score(const vector<field>& fields);
	double get_value (vector<field> fd);

public:
	maxfields(draw_tools dts, run_timer rtm, int calc, const vector<field> a, int ss);
	struct score search_fields(vector<field> current, const field& newField, int start, int max, double balance);

};

maxfields::maxfields(draw_tools dts, run_timer rtm, int calc, const vector<field> a, int ss)
{
	dt = dts;
	rt = rtm;
	calculation_type = calc;
	// this can be a shallow copy.  is this right?
	all = a;
	sameSize = ss;
}

int maxfields::cached_mu (field f)
{
	if (mucache.count(f))
		return mucache[f];

	mucache[f] = field_factory::get_instance()->get_cache_mu(f);
	return mucache[f];
}

string maxfields::draw_fields(const vector<field>& f)
{

	dt.erase();

	for (field fi: f)
		dt.add(fi);

	return dt.to_string();
}

double maxfields::calculate_balance_score(const vector<field>& fields) 
{
	unordered_map<point, int> link_counts;

	for (field field : fields) {
		vector<point> portals = field.get_points();
		for (point portal : portals) {
			if (link_counts.find(portal) == link_counts.end())
				link_counts[portal]= 1;
			else
				link_counts[portal]= link_counts[portal] + 1;
		}
	}

	int totalLinks = 0;
	int totalPortals = link_counts.size();
	for (pair<point,int> count : link_counts) {
		totalLinks += count.second;
	}

	double mean = (double) totalLinks / totalPortals;
	double variance = 0.0;

	for (pair<point,int> count : link_counts) {
		variance += pow(count.second - mean, 2);
	}

	variance /= totalPortals;
	return sqrt(variance); // return the standard deviation as the balance score
}

double maxfields::get_value (vector<field> fd)
{
	field_factory* ff = field_factory::get_instance();
	double total = 0;

	for (field f : fd)
		if (calculation_type == 0)
			total += f.geo_area();
		else
			total += ff->get_cache_mu(f);

	return total;
}

struct score maxfields::search_fields(vector<field> current, const field& newField, int start, int max, double balance)
{
	current.push_back(newField);
	if (current.size() > 0)
	{
		int newSize = current.size();
		double dispSize = get_value(current);

		if (newSize > max || (sameSize != 0 && newSize == max)) {

			double bal = calculate_balance_score (current);
			if (sameSize == 2)
				bal = dispSize;
			// want to maximise geo or mu 
			if (newSize > max || (sameSize == -1 && bal > balance) || ( sameSize == 1 && bal < balance) || (sameSize == 2 && bal > balance))
			{
				cerr << bal << " : " << newSize << " : " << dispSize << " : "  << rt.split() << " seconds." << endl;
				cout << draw_fields(current) << endl; 
				cerr << endl;
				max = newSize;
				balance = bal;
			}
		}
	}

	for (int i=start; i<all.size(); i++)
	{
		field thisField = all.at(i);
		if (!thisField.intersects(current))
		{

			struct score res = search_fields(current, thisField, i+1, max, balance);

			max = res.count;
			balance = res.balance;
		}
	}
	struct score response;
	response.count = max;
	response.balance = balance;
	return response;

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

vector<line> filter_lines (const vector<line>& li, const vector<silicontrip::link>& links, const team_count& tc, const vector<portal>& avoid_double, bool limit2k) 
{
	link_factory* lf = link_factory::get_instance();
    vector<line> la = lf->filter_links(li, links, tc);
    if (avoid_double.size() > 0)
        la = lf->filter_link_by_blocker(la, links, avoid_double);

    if (limit2k)
        la = lf->filter_link_by_length(la, 2000);
    
    return la;
}

void print_usage()
{
		cerr << "Usage:" << endl;
		cerr << "maxfields [options] <portal cluster> [<portal cluster> [<portal cluster>]]" << endl;
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
		cerr << " -s				Display plans that have the same size as the best found with decreasing variance" << endl;
		cerr << " -S				Same as -s but with increasing variance (can't use with -s)" << endl;
		cerr << " -M                Use MU calculation" << endl;
		cerr << " -T <lat,lng,...>  Use only fields covering target points" << endl;
}

int main (int argc, char* argv[])
{
	run_timer rt;

	vector<point>target;
	int calc = 0;  // area or mu
	int same_size = 0; // same calc
	vector<portal>avoid_double;
	vector<portal>avoid_single;
	vector<portal>ignore_links;
	bool limit2k = false;

	arguments ag(argc,argv);

	ag.add_req("E","enlightened",true); // max enlightened blockers
	ag.add_req("R","resistance",true); // max resistance blockers
	ag.add_req("N","machina",true); // max machina blockers
	ag.add_req("i","ignore",true); // ignore links from these portals (about to decay or easy to destroy)
	ag.add_req("a","avoid", true); // avoid using these portals.  S in other tools
	ag.add_req("k","limit2k",false); // limit link length to that can be made under fields.
	ag.add_req("D","blockers",true); // remove links with blocker using these portals.

	
	ag.add_req("C","colour",true); // drawtools colour
	ag.add_req("I","intel",false); // output as intel
	ag.add_req("L","polyline",false); // output as polylines
	ag.add_req("M","MU",false); // calculate as MU
	ag.add_req("S","same",false); // display same size plans
	ag.add_req("s","samesmall",false); // display same size plans
	ag.add_req("G","geo",false); // display same size plans

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

	if (ag.has_option("M"))
		calc = 1;

	if (ag.has_option("k"))
		limit2k=true;

	portal_factory* pf = portal_factory::get_instance();
	link_factory* lf = link_factory::get_instance();
	field_factory* ff = field_factory::get_instance();

	if (ag.has_option("s"))
		same_size = 1;

	if (ag.has_option("S"))
	{
		if (same_size == 0)
		{
			same_size = -1;
		}
		else
		{
				cerr << "Cannot have -s and -S" << endl << endl;
				print_usage();
				exit(1);
		}
	}
	if (ag.has_option("G"))
	{
		same_size = 2;
	}

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
        
        li = filter_lines(li, links, tc, avoid_double, limit2k);
        
        cerr << "== " << li.size() << " links generated " << rt.split() << " seconds. Generating fields ==" << endl;

        all_fields = ff->make_fields_from_single_links(li);
    } else if (ag.argument_size() == 2) {
        vector<line> li1 = filter_lines(lf->make_lines_from_single_cluster(clusters[0]), links, tc, avoid_double, limit2k);
        cerr << "== cluster 1 links:  " << li1.size() << " ==" << endl;

        vector<line> li2 = filter_lines(lf->make_lines_from_double_cluster(clusters[0], clusters[1]), links, tc, avoid_double, limit2k);
        cerr << "== cluster 2 links:  " << li2.size() << " ==" << endl;

        all_fields = ff->make_fields_from_double_links(li2, li1);
    } else if (ag.argument_size() == 3) {
        vector<line> li1 = filter_lines(lf->make_lines_from_double_cluster(clusters[0], clusters[1]), links, tc, avoid_double, limit2k);
        cerr << "== cluster 1 links:  " << li1.size() << " ==" << endl;

        vector<line> li2 = filter_lines(lf->make_lines_from_double_cluster(clusters[1], clusters[2]), links, tc, avoid_double, limit2k);
        cerr << "== cluster 2 links:  " << li2.size() << " ==" << endl;

        vector<line> li3 = filter_lines(lf->make_lines_from_double_cluster(clusters[2], clusters[0]), links, tc, avoid_double, limit2k);
        cerr << "== cluster 3 links:  " << li3.size() << " ==" << endl;

        all_fields = ff->make_fields_from_triple_links(li1, li2, li3);
    }

	cerr << "== " << all_fields.size() << " fields generated " << rt.split() << " seconds ==" << endl;

    // Common field processing

	if (ag.has_option("E") || ag.has_option("R"))
	    all_fields = ff->filter_existing_fields(all_fields, links); // need to make this optional. Or related to team blockers.

    all_fields = ff->filter_fields(all_fields, links, tc);

	if (!target.empty())
	{
		all_fields = ff->over_target(all_fields,target);
	}

	cerr << "== " << all_fields.size() << " fields filtered " << rt.split() << " seconds ==" << endl;
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

	vector<pair<double,string>> plan;
	int bestbest = 0;

	vector<field> search;
		//search.push_back(tfi);
	maxfields mf = maxfields(dt,rt,calc,all_fields,same_size);
	field fi = all_fields[0];
	struct score result = mf.search_fields(search, fi, 1, 0, 0.0);
	//search_fields(dt,search,all_fields,0,0,calc,same_size,0.0,rt);

	cerr << "==  plans searched " << rt.split() << " seconds ==" << endl;
	cerr <<  "== show all plans ==" << endl;

	sort (plan.begin(), plan.end(), pair_sort);
	for (pair<double,string> entry: plan) 
	{
		cout <<  entry.first << " " << entry.second << endl <<endl;
	}

	cerr <<  "== Finished. " << rt.split() << " elapsed time. " << rt.stop() << " total time." << endl;

	} catch (exception &e) {
		cerr << "An Error occured: " << e.what() << endl;
	}

	return 0;	
}
