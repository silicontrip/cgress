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

void print_usage()
{
		cerr << "Usage:" << endl;
		cerr << "layerlinker [options] <portal cluster> [<portal cluster> [<portal cluster>]]" << endl;
		cerr << "    if two clusters are specified, 2 portals are chosen to make links in the first cluster." << endl;
		cerr << "Options:" << endl;
		cerr << " -E <number>       Limit number of Enlightened Blockers" << endl;
		cerr << " -R <number>       Limit number of Resistance Blockers" << endl;
		cerr << " -N <number>       Limit number of Machina Blockers" << endl;

		cerr << " -C <#colour>      Set Drawtools output colour" << endl;
		cerr << " -L                Set Drawtools to output as polylines" << endl;
		cerr << " -O                Output as Intel Link" << endl;
		cerr << " -s				Display plans that have the same size as the best found with decreasing variance" << endl;
		cerr << " -S				Same as -s but with increasing variance (can't use with -s)" << endl;
		cerr << " -T <lat,lng,...>  Use only fields covering target points" << endl;
}

struct score {
	int count;
	double balance;
};

unordered_map<field,int> mucache;

int cached_mu (field f)
{
	if (mucache.count(f))
		return mucache[f];

	mucache[f] = field_factory::get_instance()->get_est_mu(f);
	return mucache[f];
}

string draw_fields(const vector<field>& f,draw_tools dt)
{

	dt.erase();

	for (field fi: f)
		dt.add(fi);

	return dt.to_string();
}

double calculate_balance_score(const vector<field>&  fields) 
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

struct score search_fields(draw_tools dt, const vector<field>& current, const vector<field>& all, int start, int max, int calc, int ss, double balance, run_timer rt)
{
	if (current.size() > 0)
	{
		int newSize = current.size();
		double dispSize = 0.0;
		for (field f: current)
			if (calc == 0)
				dispSize += f.geo_area();
			else 
				dispSize += cached_mu(f);

		if (newSize > max || (ss != 0 && newSize == max)) {

			double bal = calculate_balance_score (current);
			if (newSize > max || (ss == -1 && bal > balance) || ( ss == 1 && bal < balance) )
			{
				cout << bal << " : " << newSize << " : " << dispSize << " : " << draw_fields(current,dt) << endl; 
				cerr << rt.split() << " seconds." << endl;
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
			vector<field> newList;
			newList.insert(newList.end(), current.begin(), current.end());
			newList.push_back(thisField);

			struct score res = search_fields(dt, newList, all, i+1, max, calc,ss,balance,rt);

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

int main (int argc, char* argv[])
{
	run_timer rt;

	vector<point>target;
	int calc = 0;  // area or mu
	int same_size = 0; // same calc

	arguments ag(argc,argv);

	ag.add_req("E","enlightened",true); // max enlightened blockers
	ag.add_req("R","resistance",true); // max resistance blockers
	ag.add_req("N","machina",true); // max machina blockers
	
	ag.add_req("C","colour",true); // drawtools colour
	ag.add_req("O","polylines",false); // output as polylines
	ag.add_req("L","intel",false); // output as intel
	ag.add_req("M","MU",false); // calculate as MU
	ag.add_req("S","same",false); // display same size plans
	ag.add_req("s","samesmall",false); // display same size plans
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
	if (ag.has_option("O"))
		dt.set_output_as_intel();

	if (ag.has_option("M"))
		calc = 1;

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

	list<field> field_list;

	for (field f: all_fields)
		field_list.push_back(f);


	vector<field> search;
		//search.push_back(tfi);
	struct score result = search_fields(dt,search,all_fields,0,0,calc,same_size,0.0,rt);

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