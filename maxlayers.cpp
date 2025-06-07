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

class maxlayers {


private:

	draw_tools dt;
	vector<field> all;
	int calculation_type;
	int layer_limit;
	run_timer rt;
	double threshold;
	int link_limit;
	bool splits;

	double get_value (vector<field> fd);
	string draw_fields(vector<field> f);
	double search_fields(vector<field>current, const field& new_field, const vector<field>& field_list, int start, double max);
	bool add_matching(const field& current, vector<field>& existing);
	int count_links(const vector<field>& fields);

public:
	maxlayers(const draw_tools& d, const vector<field>& a, int c, int ll, const run_timer& r, double t, int link, bool s);
	vector<pair<double,string>> start_search();
};

maxlayers::maxlayers(const draw_tools& d, const vector<field>& a, int c, int layer, const run_timer& r, double t, int link, bool s)
{
	dt = d;
	all = a;
	calculation_type = c;
	layer_limit = layer;
	rt = r;
	threshold = t;
	link_limit = link;
	splits = s;
}

string maxlayers::draw_fields(vector<field> f)
{

	dt.erase();

	if (splits)
		f = field_factory::get_instance()->add_splits(f);

	for (field fi: f)
		dt.add(fi);

	return dt.to_string();
}

int maxlayers::count_links(const vector<field>& fields) 
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

	int maxLinks = 0;
	for (pair<point,int> count : link_counts) {
		if (count.second> maxLinks)
			maxLinks = count.second;
	}

	return maxLinks;
}

double maxlayers::get_value (vector<field> fd)
{
	field_factory* ff = field_factory::get_instance();
	double total = 0;
	if (splits)
		fd = ff->add_splits(fd);

	for (field f : fd)
		if (calculation_type == 0)
			total += f.geo_area();
		else
			total += ff->get_cache_mu(f);

	return total;
}

// I should make use of pass by copy and add the new field as an argument
double maxlayers::search_fields(vector<field>current, const field& new_field, const vector<field>& field_list, int start, double max)
{
	current.push_back(new_field);
	if (layer_limit > 0 && current.size() > layer_limit)
		return max;
	if (link_limit > 0)
	{
		int mx = count_links(current);
		if (mx > link_limit)
			return max;
	}
	if (current.size() > 0)
	{
		double newSize = get_value(current);

		if (newSize > max) {
			cerr << newSize << " : " << current.size() << " : " << rt.split() << " seconds." << endl;
			cout << draw_fields(current) << endl << endl; 
			max = newSize;
		}
	}

	for (int i=start; i<field_list.size(); i++)
	{
		field this_field = field_list.at(i);
		if (!this_field.intersects(current))
			max = search_fields(current, this_field, field_list, i+1, max);	
	}
		
	return max;

}

bool maxlayers::add_matching(const field& current, vector<field>& existing)
{
	bool added = false;
	for (field sfi: all)
	{
		if (current.difference(sfi) < threshold && current.inside(sfi))
		{
			bool found = false;
			for (field cfi: existing)
			{
				if (cfi == sfi)
				{
					found = true;
					break;
				}
			}
			if (!found)
			{
				existing.push_back(sfi);
				added = true;
			}
		}

	}
	return added;
}

vector<pair<double,string>> maxlayers::start_search ()
{
	vector<pair<double,string>> plan;
	double bestbest = 0.0;

	list<field> field_list;

	for (field f: all)
		field_list.push_back(f);

	//cerr << "==  fields linked " << rt.split() << " seconds ==" << endl;


	list<field>::iterator i=field_list.begin();
	while (i !=field_list.end()) 
	{
		field tfi = *i;
		vector<field> fc;

		//cerr << "== searching for similar fields ==" << endl; 
		bool added = add_matching(tfi,fc);
		while (added) 
		{
			added = false;
			for (field sfi: fc)
			{
				if(add_matching(sfi,fc))
					added = true;
			}	

		}
		//cerr << "== found " << fc.size() << " fields " << rt.split() << " seconds ==" << endl;

		for (field sfi: fc)
		{
			list<field>::iterator j=field_list.begin();
			while (j != field_list.end())
			{
				field ffi = *j;
				if (sfi == ffi)
				{
					j = field_list.erase(j);
					break;
				}
				++j;
			}
		}
		//cerr << "== found " << fc.size() << " fields in " << rt.split() << " seconds. ==" << endl; 

		vector<field> search;
		// an unexpected side effect of using pass by copy
		for (int i=0; i < fc.size(); i++)
		{
			field tfi = fc[i];
			bestbest = search_fields(search,tfi,fc,i+1,bestbest);
		}

		if (search.size() > 0)
		{
			pair<double,string> ps;

			ps.first = bestbest;
			ps.second = " ("+ std::to_string(search.size())+") / " + draw_fields(search);
			plan.push_back(ps);
			cout << bestbest << " (" << search.size() << ") / " << draw_fields(search) << endl;
			cerr  << rt.split() << " seconds." << endl;
		}
		if (fc.size() > 0)
			i=field_list.begin();
		else
			i++;
	}
	return plan;
}


bool geo_comparison(const field& a, const field& b)
{
    return a.geo_area() > b.geo_area();
}

bool pair_sort(const pair<double,string>& a, const pair<double,string>& b)
{
	return a.first < b.first;
}

vector<line> filter_lines (const vector<line>& li, const vector<silicontrip::link>& links, const team_count& tc, const vector<portal>& avoid_double, bool limit2k, double percentile) 
{
	link_factory* lf = link_factory::get_instance();
    vector<line> la = lf->filter_links(li, links, tc);
    if (avoid_double.size() > 0)
        la = lf->filter_link_by_blocker(la, links, avoid_double);

    if (limit2k)
        la = lf->filter_link_by_length(la, 2000);

    if (percentile < 100)
        la = lf->percentile_lines(la, percentile);
    
    return la;
}

vector<portal> cluster_and_filter_from_description(const vector<portal>& remove, const string desc) 
{
	portal_factory* pf = portal_factory::get_instance();
    vector<portal> portals = pf->cluster_from_description(desc);
    if (remove.size() > 0)
        portals = pf->remove_portals(portals, remove);
    return portals;
}

void print_usage()
{
		cerr << "Usage:" << endl;
		cerr << "maxlayers [options] <portal cluster> [<portal cluster> [<portal cluster>]]" << endl;
		cerr << "    if two clusters are specified, 2 portals are chosen to make links in the first cluster." << endl;
		cerr << "Generates a set of layered fields, for a given portal cluster description. Maximises geo area or MU." << endl;
		cerr << endl << "Portals clusters may be from a file, starting with './'" << endl;
		cerr << "    from a drawtools polygon starting with '[{'" << endl;
		cerr << "    from an S2Cell Id starting with '0x'" << endl;
		cerr << "    two portals separated by '=' for a lat long rectangle" << endl;
		cerr << "    three portals separated by '=' for a triangle" << endl;
		cerr << "    one portal followed by a ':' and a km range" << endl;
		cerr << "    Portals may be specified by title, guid or lat,lng." << endl;
		cerr << "    Cluster descriptions which specify regions can use arbitrary lat,lng points." << endl;
		cerr << endl;
		cerr << "Options:" << endl;
		cerr << " -E <number>       Limit number of Enlightened Blockers" << endl;
		cerr << " -R <number>       Limit number of Resistance Blockers" << endl;
		cerr << " -N <number>       Limit number of Machina Blockers" << endl;
		cerr << " -D <cluster>      Filter links crossing blockers using these portals" << endl;
		cerr << " -S <cluster>      Avoid linking to these portals" << endl;
		cerr << " -i <cluster>      Ignore blocking links from these portals" << endl;

		cerr << " -C <#colour>      Set Drawtools output colour" << endl;
		cerr << " -L                Set Drawtools to output as polylines" << endl;
		cerr << " -I                Output as Intel Link" << endl;
		cerr << " -M                Use MU calculation" << endl;
		cerr << " -t <number>       Threshold for similar fields (larger less similar)" << endl;
		cerr << " -l <number>       Maximum number of layers in plan" << endl;
		cerr << " -P <number>       Maximum number of links from a single portal" << endl;
		cerr << " -k                Limit links to 2km" << endl;
		cerr << " -s                Add split fields" << endl;
		cerr << " -p <percentile>   Use longest percentile links" << endl;
		cerr << " -f <percentile>   Use largest percentile fields" << endl;
		cerr << " -T <lat,lng,...>  Use only fields covering target points" << endl;
}

int main (int argc, char* argv[])
{
	run_timer rt;

	int mlayers = 0;
	int mlinks = 0;
	double threshold;
	double percentile = 100;
	double fpercentile = 100;
	vector<point>target;
	vector<portal>avoid_double;
	vector<portal>avoid_single;
	vector<portal>ignore_links;
	int calc = 0;  // area or mu
	bool splits = false;
	bool limit2k = false;

	arguments ag(argc,argv);

	ag.add_req("E","enlightened",true); // max enlightened blockers
	ag.add_req("R","resistance",true); // max resistance blockers
	ag.add_req("N","machina",true); // max machina blockers
	ag.add_req("D","blockers",true); // remove links with blocker using these portals.
	ag.add_req("S","avoid", true); // avoid using these portals.
	ag.add_req("i","ignore",true); // ignore links from these portals (about to decay or easy to destroy)
	
	ag.add_req("C","colour",true); // drawtools colour
	ag.add_req("I","intel",false); // output as intel
	ag.add_req("L","polylines",false); // output as polylines
	ag.add_req("M","MU",false); // calculate as MU
	//ag.add_req("m","",true); // maximum size
	ag.add_req("t","threshold",true); // field similar threshold
	ag.add_req("p","lpercent",true); // use percentile longest links
	ag.add_req("f","fpercent",true); // use percentile biggest fields
	ag.add_req("l","maxlayers",true); // maximum layers
	ag.add_req("P","maxlinks",true); // maximum links
	ag.add_req("T","target",true); // target fields over location
	ag.add_req("k","limit2k",false); // limit link length to that can be made under fields.
	ag.add_req("h","help",false);
	ag.add_req("s","splits",false);

	if (!ag.parse() || ag.has_option("h"))
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
	if (ag.has_option("I")) // right that does it, I'm changing it to I
		dt.set_output_as_intel();

	threshold = 0.2;
	if (ag.has_option("t"))
		threshold = ag.get_option_for_key_as_double("t");
	
	if (ag.has_option("p"))
		percentile = ag.get_option_for_key_as_double("p");

	if (ag.has_option("l")) {
		cerr << "max layers: " << ag.get_option_for_key_as_int("l") << endl;
		mlayers = ag.get_option_for_key_as_int("l");
	}
	if (ag.has_option("P")) {
		cerr << "max links: " << ag.get_option_for_key_as_int("P") << endl;
		mlinks = ag.get_option_for_key_as_int("P");
	}

	if (ag.has_option("M"))
		calc = 1;

	if (ag.has_option("s"))
		splits = true;

	if (ag.has_option("k"))
		limit2k=true;

	portal_factory* pf = portal_factory::get_instance();
	link_factory* lf = link_factory::get_instance();
	field_factory* ff = field_factory::get_instance();

	if (ag.has_option("T"))
		target = pf->points_from_string(ag.get_option_for_key("T"));

	if (ag.has_option("D"))
	{
		avoid_double = pf->cluster_from_description(ag.get_option_for_key("D"));
		// for (portal p: avoid_double)
		//	cerr << "avoid: " << p << endl;
	}
	if (ag.has_option("S"))
		avoid_single = pf->cluster_from_description(ag.get_option_for_key("S"));

	if (ag.has_option("i"))
		ignore_links = pf->cluster_from_description(ag.get_option_for_key("i"));
		 
	cerr << "== Reading links and portals ==" << endl;
	rt.start();

// of course I had to pick a colliding name for my class
	vector<silicontrip::link> links;
	vector<field> all_fields;
	vector<field> af;

	try {

	vector<portal> all_portals;
    vector<vector<portal>> clusters;
    
    // Load portals based on argument count
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

    // Combine all portals
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
        
        cerr << "== links generated " << rt.split() << " seconds. Generating fields ==" << endl;

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

    // Common field processing
    all_fields = ff->filter_existing_fields(all_fields, links);
    all_fields = ff->filter_fields(all_fields, links, tc);
    
    cerr << "== Fields:  " << all_fields.size() << " ==" << endl;


	
	cerr << "==  fields generated " << rt.split() << " seconds ==" << endl;

	if (target.size()>0)
	{
		all_fields = ff->over_target(all_fields,target);
	}
	if (fpercentile < 100)
	{
		all_fields = ff->percentile(all_fields,fpercentile);
	}

	cerr << "==  fields filtered " << rt.split() << " seconds ==" << endl;
	cerr << "== sorting fields ==" << endl;

	sort(all_fields.begin(),all_fields.end(),geo_comparison);

	cerr << "==  fields sorted " << rt.split() << " ==" << endl;
	cerr << "== show matches ==" << endl;

	maxlayers ml = maxlayers(dt, all_fields, calc, mlayers, rt, threshold,mlinks,splits);

	vector<pair<double,string>> plan = ml.start_search();

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
