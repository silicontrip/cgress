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
		cerr << "    if two clusters are specified, 2 links are made from the first cluster." << endl;
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

int find_field(const vector<field>& fields,int start,const field& current,const vector<field>&exist, double threshold)
{
	int best=-1;
	double closest = 9999.0;
	for (int n =start; n < fields.size(); n++)
	{
		field fi =fields.at(n);
		if (!fi.intersects(exist))
		{
			double diff = current.difference(fi);
			// want to make a field selection strategy, for different fielding plans.
			// need to make configurable threshold
			if (diff < threshold  && diff < closest)
			{
				closest = diff;
				best = n;
			}

		}
	}
	return best;
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
		threshold = 0.3;
	
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

		}
		// double and triple clusters
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
		for (int i =0; i< all_fields.size();i++ ) {
			field tfi = all_fields.at(i);
			double at = 0.0;
			if (calc==0)
			{
				at += tfi.geo_area();
			}
			else
			{
				at += ff->get_est_mu(tfi);
			}
			vector<field> fc;
			dt.erase();
			fc.push_back(tfi);
			dt.add(tfi);

			int best = find_field(all_fields,i+1,tfi,fc,threshold);

			while (best != -1 && (maxlayers==0 || fc.size() < maxlayers)) 
			{
				tfi = all_fields.at(best);
				if (calc==0)
				{
					at += tfi.geo_area();
					dt.add(tfi);
					fc.push_back(tfi);
				}
				else
				{
					//System.out.println ("at: " + at  + " est: " + tfi.getEstMu());
					// if (at + tfi.get_est_mu() < notOver)
					// {
							at += ff->get_est_mu(tfi);
							dt.add(tfi);
							fc.push_back(tfi);
					//	}
				}
				best = find_field(all_fields,best+1,tfi,fc,threshold); 
			}

			pair<double,string> ps;

			ps.first = at;
			ps.second = " ("+ std::to_string(fc.size())+") / " + dt.to_string();
			plan.push_back(ps);
			if (at>bestbest) {
				bestbest = at;
				cout << at << " (" << fc.size() << ") / " << dt.to_string() << endl;
				cerr << "split: " << rt.split() << endl;
			}
		}
		cerr << "==  plans searched " << rt.split() << " seconds ==" << endl;
		cerr <<  "== show all plans ==" << endl;

		sort (plan.begin(), plan.end(), pair_sort);
		for (pair<double,string> entry: plan) 
		{
			cout <<  entry.first << " " << entry.second << endl <<endl;
		}

		cerr <<  "== Finished. " << rt.split() << " elapsed time. " << rt.stop() << " total time." << endl;

		return 0;
	}
