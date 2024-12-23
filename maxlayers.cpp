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
		cerr << " -M                Use MU calculation" << endl;
		cerr << " -t <number>       Threshold for similar fields (larger less similar)" << endl;
		cerr << " -l <number>       Maximum number of layers in plan" << endl;
		cerr << " -p <percentile>   Use longest percentile links" << endl;
		cerr << " -f <percentile>   Use largest percentile fields" << endl;
		cerr << " -T <lat,lng,...>  Use only fields covering target points" << endl;
}

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

double search_fields(draw_tools dt, const vector<field>& current, const vector<field>& all, int start, double max, int calc, int layerLimit, run_timer rt)
{
	if (current.size() > layerLimit)
		return max;
	if (current.size() > 0)
	{
		double newSize = 0.0;
		for (field f: current)
			if (calc == 0)
				newSize += f.geo_area();
			else 
				newSize += cached_mu(f);

		if (newSize > max) {
			cout << newSize << " : " << current.size() << " : " << draw_fields(current,dt) << endl; 
			cerr << rt.split() << " seconds." << endl;
			max = newSize;
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

			max = search_fields(dt, newList, all, i+1, max, calc,layerLimit,rt);	
		}
	}
		
	return max;

}

bool add_matching(const list<field>& all, const field& current, vector<field>& existing, double threshold)
{
	bool added = false;
	for (field sfi: all)
	{
		if (current.difference(sfi) < threshold)
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

	int maxlayers = 0;
	double threshold;
	double percentile = 100;
	double fpercentile = 100;
	vector<point>target;
	int calc = 0;  // area or mu

	arguments ag(argc,argv);

	ag.add_req("E","enlightened",true); // max enlightened blockers
	ag.add_req("R","resistance",true); // max resistance blockers
	ag.add_req("N","machina",true); // max machina blockers
	
	ag.add_req("C","colour",true); // drawtools colour
	ag.add_req("O","polylines",false); // output as polylines
	ag.add_req("L","intel",false); // output as intel
	ag.add_req("M","MU",false); // calculate as MU
	ag.add_req("m","",true); // maximum size
	ag.add_req("t","threshold",true); // field similar threshold
	ag.add_req("p","",true); // use percentile longest links
	ag.add_req("f","",true); // use percentile biggest fields
	ag.add_req("l","maxlayers",true); // maximum layers
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

	if (ag.has_option("t"))
		threshold = ag.get_option_for_key_as_double("t");
	else 
		threshold = 0.2;
	
	if (ag.has_option("p"))
		percentile = ag.get_option_for_key_as_double("p");

	if (ag.has_option("l")) {
		cerr << "max layers: " << ag.get_option_for_key_as_int("l") << endl;
		maxlayers = ag.get_option_for_key_as_int("l");
	}
	if (ag.has_option("M"))
		calc = 1;

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
		if (percentile < 100)
			li = lf->percentile_lines(li,percentile);
					
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
	if (fpercentile < 100)
	{
		all_fields = ff->percentile(all_fields,fpercentile);
	}

	cerr << "==  fields filtered " << rt.split() << " seconds ==" << endl;
	cerr << "== sorting fields ==" << endl;

	sort(all_fields.begin(),all_fields.end(),geo_comparison);

	cerr << "==  fields sortered " << rt.split() << " ==" << endl;
	cerr << "== show matches ==" << endl;

	vector<pair<double,string>> plan;
	double bestbest = 0.0;

	list<field> field_list;

	for (field f: all_fields)
		field_list.push_back(f);

	list<field>::iterator i=field_list.begin();
	while (i !=field_list.end()) 
	{
		field tfi = *i;
		vector<field> fc;

		//cerr << "== searching for similar fields ==" << endl; 
		bool added = add_matching(field_list,tfi,fc,threshold);
		while (added) 
		{
			added = false;
			for (field sfi: fc)
			{
				if(add_matching(field_list,sfi,fc,threshold))
					added = true;
			}	

		}
		//cerr << "== deleting. ==" << endl; 

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
		//search.push_back(tfi);
		bestbest = search_fields(dt,search,fc,0,bestbest,calc,maxlayers,rt);

		if (search.size() > 0)
		{
			pair<double,string> ps;

			ps.first = bestbest;
			ps.second = " ("+ std::to_string(search.size())+") / " + draw_fields(search,dt);
			plan.push_back(ps);
			cout << bestbest << " (" << search.size() << ") / " << draw_fields(search,dt) << endl;
			cerr << "split: " << rt.split() << endl;
		}
		if (fc.size() > 0)
			i=field_list.begin();
		else
			i++;
	}
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
