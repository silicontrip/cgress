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

class cellfields {

private:
	unordered_map<field,int> mucache;
	draw_tools dt;
	run_timer rt;
	vector<field> all;
	string cell_token;
	//S2CellId cellid;
	field_factory* ff;
	int limit_layers;

	string draw_fields(const vector<field>& f);
	double calc_score(const field& f) const;
	double search_fields(vector<field> current, const field& f, int start, double best);

public:
	cellfields(draw_tools dts, run_timer rtm, string tok, int l);
	double start_search(double best, const vector<field>&af);


};

cellfields::cellfields(draw_tools dts, run_timer rtm, string tok, int l)
{
	dt = dts;
	rt = rtm;
	// this can be a shallow copy.  is this right?
	//all = a;
	cell_token = tok;
	//cellid = S2CellId::FromToken(tok);
	ff = field_factory::get_instance();
	limit_layers = l;
}

string cellfields::draw_fields(const vector<field>& f)
{

	dt.erase();

	for (field fi: f)
		dt.add(fi);

	return dt.to_string();
}

double cellfields::calc_score(const field& f) const
{
	unordered_map<string,double> intersections = ff->cell_intersection(f);
	vector<string> cells = ff->celltokens(f);
	unordered_map<string,uniform_distribution> cellmu = ff->query_mu(cells);  // caching handled by field_factory

	double farea = f.geo_area();
	double area = intersections[cell_token];	

	//uniform_distribution others = uniform_distribution(0,0);
	double other_range = 1.0;
	for (pair<string,double> ii : intersections)
	{
		if (ii.first != cell_token)
		{
			uniform_distribution this_cell = cellmu[ii.first]; // handle undefined
			//cerr << ii.first.ToToken() << ": " << this_cell << endl;
			uniform_distribution mu_intersection = this_cell * ii.second;
			//others += mu_intersection;
			other_range += mu_intersection.range();
		}
	}


	// not sure about this method
	uniform_distribution fieldmu = cellmu[cell_token] * area;

	//uniform_distribution inverse = others.inverse();
	//uniform_distribution score = fieldmu - others.inverse();

	//cerr << "others: " << others << " score: " << score << endl;
	// cerr << "other range: " << other_range <<endl;
	return fieldmu.range() / other_range;

}

double cellfields::start_search(double best, const vector<field>& af)
{
	all = af;
	vector<field> current;
	for (int i=0; i<all.size(); i++)
	{
		field thisField = all.at(i);
		double res = search_fields(current, thisField, i+1, best);
		if (res > best)
			best = res;
	}
	return best;
}

double cellfields::search_fields(vector<field> current, const field& f, int start, double best)
{
	if (calc_score(f) > 1.0)
		current.push_back(f);
	else
		return best;

	if (current.size() > 0)
	{
		int newSize = current.size();
		if (limit_layers > 0 && newSize > limit_layers)
			return best;
		double total_score = 0;
		for (field fi : current)
			total_score += calc_score(fi);

		if (total_score > best) {

				cerr << total_score << " : " << newSize << " : " << rt.split() << " seconds." << endl;
				cout << draw_fields(current) << endl; 
				cerr << endl;
				best = total_score;
			
		}
	}

	// area required cell / sum(mu other cells)

	for (int i=start; i<all.size(); i++)
	{
		field thisField = all.at(i);
		if (!thisField.intersects(current))
		{
			//vector<field> newList;
			//newList.insert(newList.end(), current.begin(), current.end());
			//newList.push_back(thisField);

			double res = search_fields(current, thisField, i+1, best);
			if (res > best)
				best = res;

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

void print_usage()
{
		cerr << "Usage:" << endl;
		cerr << "maxfields [options] -c <cellid> [<portal cluster>]" << endl;
		cerr << "Generates fields for specified cell." << endl;
		cerr << "Options:" << endl;
		cerr << " -E <number>       Limit number of Enlightened Blockers" << endl;
		cerr << " -R <number>       Limit number of Resistance Blockers" << endl;
		cerr << " -N <number>       Limit number of Machina Blockers" << endl;
		cerr << " -S <cluster>      Avoid linking to these portals" << endl;
		cerr << " -f <number>       Make this many fields." << endl;
		cerr << " -c <cell id>      Use this cell. Required." << endl;
		cerr << " -l <number>       limit fields to no more than this." << endl;
		cerr << " -C <#colour>      Set Drawtools output colour" << endl;
		cerr << " -L                Set Drawtools to output as polylines" << endl;
		cerr << " -I                Output as Intel Link" << endl;
}

double max_dist (unordered_set<S2CellId> cellids, S2CellId centre)
{
	S2LatLng cenll = centre.ToLatLng();

	S1Angle adist = S1Angle::Radians(0);

	for (S2CellId cid : cellids)
	{
		S2LatLng cill = cid.ToLatLng();
		S1Angle cdist = cill.GetDistance(cenll);
		if (cdist > adist)
			adist = cdist;
	}

	return adist.radians() * point::earth_radius; 
}

unordered_set<S2CellId> expand_cells(unordered_set<S2CellId> cellids, S2CellId centre)
{

	//   void GetEdgeNeighbors(S2CellId neighbors[4]) const;
	unordered_set<S2CellId> new_cells;
	S2LatLng cenll = centre.ToLatLng();

	int csize = cellids.size();

	double current = max_dist(cellids, centre);

	while (new_cells.size() <= csize)
	{
		for (S2CellId cid : cellids)
		{
			new_cells.insert(cid);
			S2CellId neighbors[4];
			cid.GetEdgeNeighbors(neighbors);
			for (S2CellId icid : neighbors) {
				S1Angle ndist = icid.ToLatLng().GetDistance(cenll);
				if (ndist.radians() * point::earth_radius < current)
					new_cells.insert(icid);
			}
		}
		current += 0.1;
	}

	return new_cells;
}

vector<line> filter_lines (const vector<line>& li, const vector<silicontrip::link>& links, const team_count& tc) 
{
	link_factory* lf = link_factory::get_instance();
    vector<line> la = lf->filter_links(li, links, tc);
    
    return la;
}

vector<portal> cluster_and_filter_from_cell_set(const vector<portal>& remove, const unordered_set<S2CellId>& cellids) 
{
	portal_factory* pf = portal_factory::get_instance();
	vector<portal> all_portals;
	for (S2CellId cid : cellids)
	{
		string desc = "0x" + cid.ToToken();
		//cerr << "Getting portals from cell: " << desc << endl;
     	vector<portal> cluster = pf->cluster_from_description(desc);
		all_portals.insert(all_portals.end(), cluster.begin(), cluster.end());
	}
    if (remove.size() > 0)
        all_portals = pf->remove_portals(all_portals, remove);
    return all_portals;
}

vector<portal> cluster_and_filter_from_description(const vector<portal>& remove, const string desc) 
{
	portal_factory* pf = portal_factory::get_instance();
    vector<portal> portals = pf->cluster_from_description(desc);
    if (remove.size() > 0)
        portals = pf->remove_portals(portals, remove);
    return portals;
}

int main (int argc, char* argv[])
{
	run_timer rt;

	vector<point>target;

	vector<portal>avoid_single;
	string cellid;
	int limit=0;
	int fields=100;

	arguments ag(argc,argv);

	ag.add_req("E","enlightened",true); // max enlightened blockers
	ag.add_req("R","resistance",true); // max resistance blockers
	ag.add_req("N","machina",true); // max machina blockers
	ag.add_req("S","avoid", true); // avoid using these portals.
	ag.add_req("f","fields",true); 
	ag.add_req("C","colour",true); // drawtools colour
	ag.add_req("I","intel",false); // output as intel
	ag.add_req("L","polyline",false); // output as polylines
	ag.add_req("l","limit",true); // limit layers/fields
	ag.add_req("c","cellid",true); // generate plan for this cell
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

	if (ag.has_option("l"))
		limit = ag.get_option_for_key_as_int("l");

	if (ag.has_option("f"))
		fields = ag.get_option_for_key_as_int("f");

	if (ag.has_option("c"))
		cellid = ag.get_option_for_key("c");

	if (cellid.length() == 0)
	{
		print_usage();
		exit(1);
	}

	S2CellId s2cellid = S2CellId::FromToken(cellid);
	S2Cell s2cell = S2Cell(s2cellid);

	S2Point s2point = s2cell.GetCenter();
	S2LatLng s2latlng = S2LatLng(s2point);

	cout << "Cell Centre: " << s2latlng.lat() <<","<<s2latlng.lng() << endl;

	portal_factory* pf = portal_factory::get_instance();
	link_factory* lf = link_factory::get_instance();
	field_factory* ff = field_factory::get_instance();

	if (ag.has_option("S"))
		avoid_single = pf->cluster_from_description(ag.get_option_for_key("S"));

	cerr << "== Reading links and portals ==" << endl;
	rt.start();

// of course I had to pick a colliding name for my class
	vector<silicontrip::link> links;
	vector<field> all_fields;
	//vector<field> af;

	vector<portal> all_portals;
    vector<vector<portal>> clusters;

	try {
	if (ag.argument_size() == 0)
	{
		// ooo fun
		//   void GetEdgeNeighbors(S2CellId neighbors[4]) const;
		cellfields cf = cellfields(dt,rt,cellid,limit);
		
		unordered_set<S2CellId> search_cells;
		search_cells.insert(s2cellid);
		double best = 0;
		int old_fields = -1;
		while (true) // whats a good value here...
		{
			stringstream cluster_desc;

			//cluster_desc << cell_centre.lat() << "," << cell_centre.lng() << ":" << range;

			//cerr << "Query: " << cluster_desc.str() << endl;

			vector<portal> portals = cluster_and_filter_from_cell_set(avoid_single, search_cells);

			cerr << "== " << portals.size() << " portals read. in " << rt.split() << " seconds. ==" << endl;
			if (portals.size() > 2)
			{
				//cerr << "== getting links ==" << endl;
										
				links = lf->get_purged_links(portals);
										
				//cerr <<  "== " << links.size() << " links read. in " << rt.split() <<  " seconds ==" << endl;

				//cerr << "== generating potential links ==" << endl;

				vector<line> li = lf->make_lines_from_single_cluster(portals);
				//cerr << "all links: " << li.size() << endl;

				li = lf->filter_links(li,links,tc);
							
				//cerr << "purged links: " << li.size() << endl;
				cerr << "==  links generated " << rt.split() <<  " seconds ==" << endl;
				if (li.size() > 2)
				{
					//cerr << "== Generating fields ==" << endl;

					all_fields = ff->make_fields_from_single_links(li);
					all_fields = ff->filter_existing_fields(all_fields,links);
					all_fields = ff->filter_fields_with_cell(all_fields,cellid);

					all_fields = ff->filter_fields(all_fields,links,tc);
					cerr << "== fields: " << all_fields.size() << " generated in "<< rt.split() << " seconds. ==" << endl;
					if (all_fields.size()>0)
					{
						best = cf.start_search(best,all_fields);
					}
				}
			}
			search_cells = expand_cells(search_cells,s2cellid);

		}

		// cerr << "Found threshold fields." << endl;

	}	
	else if (ag.argument_size() == 1) {
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
    cerr <<  "== " << links.size() << " links read. in " << rt.split() <<  " seconds ==" << endl;
    cerr << "== generating potential links ==" << endl;

	if (ag.argument_size() == 1) {
        vector<line> li = lf->make_lines_from_single_cluster(clusters[0]);
        cerr << "all links: " << li.size() << endl;
        
        li = filter_lines(li, links, tc);
        
        cerr << "== links generated " << rt.split() << " seconds. Generating fields ==" << endl;

        all_fields = ff->make_fields_from_single_links(li);
    } else if (ag.argument_size() == 2) {
        vector<line> li1 = filter_lines(lf->make_lines_from_single_cluster(clusters[0]), links, tc);
        cerr << "== cluster 1 links:  " << li1.size() << " ==" << endl;

        vector<line> li2 = filter_lines(lf->make_lines_from_double_cluster(clusters[0], clusters[1]), links, tc);
        cerr << "== cluster 2 links:  " << li2.size() << " ==" << endl;

        all_fields = ff->make_fields_from_double_links(li2, li1);
    } else if (ag.argument_size() == 3) {
        vector<line> li1 = filter_lines(lf->make_lines_from_double_cluster(clusters[0], clusters[1]), links, tc);
        cerr << "== cluster 1 links:  " << li1.size() << " ==" << endl;

        vector<line> li2 = filter_lines(lf->make_lines_from_double_cluster(clusters[1], clusters[2]), links, tc);
        cerr << "== cluster 2 links:  " << li2.size() << " ==" << endl;

        vector<line> li3 = filter_lines(lf->make_lines_from_double_cluster(clusters[2], clusters[0]), links, tc);
        cerr << "== cluster 3 links:  " << li3.size() << " ==" << endl;

        all_fields = ff->make_fields_from_triple_links(li1, li2, li3);
    }

    // Common field processing
	all_fields = ff->filter_fields_with_cell(all_fields,cellid);
    all_fields = ff->filter_existing_fields(all_fields, links);
    all_fields = ff->filter_fields(all_fields, links, tc);

    cerr << "== Fields:  " << all_fields.size() << " ==" << endl;

	cerr << "==  fields generated " << rt.split() << " seconds ==" << endl;
	cerr << "== sorting fields ==" << endl;

	sort(all_fields.begin(),all_fields.end(),geo_comparison);

	cerr << "==  fields sorted " << rt.split() << " ==" << endl;
	cerr << "== show matches ==" << endl;

	//vector<pair<double,string>> plan;
	//int bestbest = 0;

	vector<field> search;
	//search.push_back(tfi);
	cellfields cf = cellfields(dt,rt,cellid,limit);
	double result = cf.start_search(0.0,all_fields);
	//search_fields(dt,search,all_fields,0,0,calc,same_size,0.0,rt);

	cerr << "==  plans searched " << rt.split() << " seconds ==" << endl;
	cerr <<  "== show all plans ==" << endl;

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
