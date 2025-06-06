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
	//unordered_set<field> precision_field;
	//double precision_best;
	//bool show_precision;
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
	//show_precision = p;
	//precision_best = 0;
}

string cellfields::draw_fields(const vector<field>& f)
{

	dt.erase();

	for (field fi: f)
		dt.add(fi);

	return dt.to_string();
}

uniform_distribution remaining(uniform_distribution v, const field& f, string celltok)
{
	field_factory* ff = field_factory::get_instance();

	unordered_map<string,double> intersections = ff->cell_intersection(f);
	vector<string> cells = ff->celltokens(f);
	unordered_map<string,uniform_distribution> cellmu = ff->query_mu(cells);

	for (pair<string,double> ii : intersections)
	{
		if (ii.first != celltok)
		{
			uniform_distribution this_mu (0.0,1000000.0);
			if (cellmu.count(ii.first) != 0)
				this_mu = cellmu[ii.first];
			v -= this_mu * ii.second;
		}
	}
	return v;
}

uniform_distribution other_contribution(const field& f, string celltok, const unordered_map<string,double>& intersections,const unordered_map<string,uniform_distribution>& cellmu)
{
	//field_factory* ff = field_factory::get_instance();
	//unordered_map<string,double> intersections = ff->cell_intersection(f);
	//vector<string> cells = ff->celltokens(f);
	//unordered_map<string,uniform_distribution> cellmu = ff->query_mu(cells);

	uniform_distribution othermu(0.0,0.0);
	for (pair<string,double> ii : intersections)
	{
		if (ii.first != celltok)
		{
			uniform_distribution this_mu (0.0,1000000.0);
			if (cellmu.count(ii.first) != 0)
				this_mu = cellmu.at(ii.first);
			othermu += this_mu * ii.second;
		}
	}
	return othermu;
}

uniform_distribution uniform_improvement(const field& f, uniform_distribution remain, string celltok)
{
	field_factory* ff = field_factory::get_instance();

	unordered_map<string,double> intersections = ff->cell_intersection(f);
	vector<string> cells = ff->celltokens(f);
	unordered_map<string,uniform_distribution> cellmu = ff->query_mu(cells);

	uniform_distribution othermu = other_contribution(f,celltok,intersections,cellmu);

	uniform_distribution totalmu = othermu + cellmu[celltok] * intersections[celltok];
	
	// cerr << "this field mu est: " << totalmu << endl;

	double totalmax = round(totalmu.get_upper());
	double totalmin = round(totalmu.get_lower());

    if (totalmin==0)
        totalmin=1;

    if (totalmax==0)
        totalmax=1;

	if (totalmin == totalmax)
		return remain;

	double current = cellmu[celltok].range();
    double worst = 0.0;
	uniform_distribution worst_dist;
    for (int tmu = totalmin; tmu <= totalmax; tmu++)
    {
        uniform_distribution mu(tmu-0.5,tmu+0.5);
        if (tmu==1)
            mu = uniform_distribution(0.0,1.5);
        
        uniform_distribution remain = (mu - othermu) / intersections[celltok]; //remaining(mu, f, celltok) / intersections[celltok];
        uniform_distribution intremain = remain.intersection(cellmu[celltok]);

		double intremainrange = intremain.range();
		
		if (intremainrange >= current) // might need to epsilon this
			return remain;
        if (intremainrange > worst)
		{
            worst = intremain.range();
			worst_dist = intremain;
		}
        // cerr << "mu: " << tmu << " rem: " << remain << " range: " << intremain.range() << " imp: " << cellmu[celltok].range() / intremain.range() <<   endl;

    }

	return worst_dist;
}

double multi_improvement(const vector<field>& vf, string celltok)
{
	double imp = 1.0;
	if (vf.size() > 0)
	{
		field_factory* ff = field_factory::get_instance();

		vector<string> cells = ff->celltokens(vf.at(0));
		unordered_map<string,uniform_distribution> cellmu = ff->query_mu(cells);

		uniform_distribution current = cellmu[celltok];
		for (const field& f : vf)
		{
			uniform_distribution uimp = uniform_improvement(f,current,celltok);
			imp = imp * current.range() / uimp.range();
			current = uimp;
		}
	}
	return imp;
}

void compare_improvement(const field& f1, const field& f2, string celltok)
{

	field_factory* ff = field_factory::get_instance();

	unordered_map<string,double> intersections1 = ff->cell_intersection(f1);
	vector<string> cells1 = ff->celltokens(f1);
	unordered_map<string,uniform_distribution> cellmu1 = ff->query_mu(cells1);

	uniform_distribution othermu1 = other_contribution(f1,celltok,intersections1,cellmu1);

	uniform_distribution totalmu1 = othermu1 + cellmu1[celltok] * intersections1[celltok];

	unordered_map<string,double> intersections2 = ff->cell_intersection(f2);
	vector<string> cells2 = ff->celltokens(f2);
	unordered_map<string,uniform_distribution> cellmu2 = ff->query_mu(cells2);

	uniform_distribution othermu2 = other_contribution(f2,celltok,intersections2,cellmu2);

	// cerr << "this field mu est: " << totalmu << endl;

	double totalmax1 = round(totalmu1.get_upper());
	double totalmin1 = round(totalmu1.get_lower());

    if (totalmin1==0)
        totalmin1=1;

    if (totalmax1==0)
        totalmax1=1;

	//if (totalmin1 == totalmax1)
	//	return 1.0;

	double current = cellmu1[celltok].range();
    double worst = DBL_MAX;
    for (int tmu1 = totalmin1; tmu1 <= totalmax1; tmu1++)
    {
        uniform_distribution mu1(tmu1-0.5,tmu1+0.5);
        if (tmu1==1)
            mu1 = uniform_distribution(0.0,1.5);
        
        uniform_distribution remain1 = (mu1 - othermu1) / intersections1[celltok]; //remaining(mu, f, celltok) / intersections[celltok];
        uniform_distribution intremain1 = remain1.intersection(cellmu1[celltok]);


		uniform_distribution totalmu2 = othermu2 + intremain1 * intersections2[celltok];

		double totalmax2 = round(totalmu2.get_upper());
		double totalmin2 = round(totalmu2.get_lower());

    	if (totalmin2==0)
        	totalmin2=1;

    	if (totalmax2==0)
        	totalmax2=1;

		double imp1 = cellmu1[celltok].range() / intremain1.range();
		cout << "f1: [" << tmu1 << ":" << imp1 << "] f2:";

		bool first = true;
		double worst2 = 0.0;
		for (int tmu2 = totalmin2; tmu2 <= totalmax2; tmu2++)
    	{
        	uniform_distribution mu2(tmu2-0.5,tmu2+0.5);
        	if (tmu2==1)
            	mu2 = uniform_distribution(0.0,1.5);
        
        	uniform_distribution remain2 = (mu2 - othermu2) / intersections2[celltok]; //remaining(mu, f, celltok) / intersections[celltok];
        	uniform_distribution intremain2 = remain2.intersection(intremain1);

			// intremain2 field2 improvement combined field1

			if (intremain2.range() > worst2)
				worst2 = intremain2.range();

			double imp2 = intremain1.range() / intremain2.range();

			cout << " [" << tmu2 << ":" << imp2 << "]";

		}

		double cimp = imp1 * intremain1.range() / worst2;
		
		cout << " (" << cimp << ")" << endl;

		if (cimp < worst)
			worst = cimp;

    }

	//return cellmu[celltok].range() / worst;

}

double pimprovement(const field& f, string celltok)
{

	field_factory* ff = field_factory::get_instance();

	unordered_map<string,double> intersections = ff->cell_intersection(f);
	vector<string> cells = ff->celltokens(f);
	unordered_map<string,uniform_distribution> cellmu = ff->query_mu(cells);

	uniform_distribution othermu = other_contribution(f,celltok,intersections,cellmu);

	uniform_distribution totalmu = othermu + cellmu[celltok] * intersections[celltok];
	
	// cerr << "this field mu est: " << totalmu << endl;

	double totalmax = round(totalmu.get_upper());
	double totalmin = round(totalmu.get_lower());

    if (totalmin==0)
        totalmin=1;

    if (totalmax==0)
        totalmax=1;

	if (totalmin == totalmax)
		return 1.0;

	double current = cellmu[celltok].range();
    double worst = 0.0;
    for (int tmu = totalmin; tmu <= totalmax; tmu++)
    {
        uniform_distribution mu(tmu-0.5,tmu+0.5);
        if (tmu==1)
            mu = uniform_distribution(0.0,1.5);
        
        uniform_distribution remain = (mu - othermu) / intersections[celltok]; //remaining(mu, f, celltok) / intersections[celltok];
        uniform_distribution intremain = remain.intersection(cellmu[celltok]);

		double intremainrange = intremain.range();
		
		if (intremainrange >= current) // might need to epsilon this
			return 1.0;
        if (intremainrange > worst)
            worst = intremain.range();

        // cerr << "mu: " << tmu << " rem: " << remain << " range: " << intremain.range() << " imp: " << cellmu[celltok].range() / intremain.range() <<   endl;

    }

	return cellmu[celltok].range() / worst;

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

	double fscore = pimprovement(f,cell_token);

	if (fscore <= 1.0)
		return best;

	current.push_back(f);
	
	if (current.size() > 0)
	{
		int newSize = current.size();
		if (limit_layers > 0 && newSize > limit_layers)
			return best;

		double total_score = multi_improvement(current,cell_token);
		/*
		for (field fi : current)
			total_score += pimprovement(fi,cell_token);  // there should be a better way to calculate total score

		if (current.size() == 2)
		{
			compare_improvement(current.at(0),current.at(1),cell_token);
			cout << endl;
			compare_improvement(current.at(1),current.at(0),cell_token);
			cout << "==" << endl;
		}
		*/
		if (total_score > best) {

			cerr << total_score << " : " << newSize << " : " << rt.split() << " seconds." << endl;
			cout << draw_fields(current) << endl; 
			cerr << endl;
			best = total_score;
		} else {
				return best;
		}
	}

	// area required cell / sum(mu other cells)
	if (limit_layers == 0 || current.size() < limit_layers) {
		for (int i=start; i<all.size(); i++)
		{
			field thisField = all.at(i);
			if (!thisField.intersects(current))
			{
				double res = search_fields(current, thisField, i+1, best);
				
				if (res > best)
					best = res;
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
		cerr << " -c <cell id>      Use this cell. Required." << endl;
		cerr << " -l <number>       limit fields to no more than this." << endl;
		cerr << " -p                Display precision fields regardless." << endl;
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
	// bool showp=false;

	arguments ag(argc,argv);

	ag.add_req("E","enlightened",true); // max enlightened blockers
	ag.add_req("R","resistance",true); // max resistance blockers
	ag.add_req("N","machina",true); // max machina blockers
	ag.add_req("S","avoid", true); // avoid using these portals.
	ag.add_req("C","colour",true); // drawtools colour
	ag.add_req("I","intel",false); // output as intel
	ag.add_req("L","polyline",false); // output as polylines
	ag.add_req("l","limit",true); // limit layers/fields
	// ag.add_req("p","showprecision",false);  // new scoring algorithm automatically handles both general improvement and precision improvement.
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

	if (ag.has_option("c"))
		cellid = ag.get_option_for_key("c");

	//if (ag.has_option("p"))
	//	showp=true;

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
		int iteration = 1;
		while (true) // whats a good value here...
		{
			cerr << "search iteration: " << iteration++  << endl;
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
				cerr << "==  "  << li.size() << " links generated " << rt.split() <<  " seconds ==" << endl;
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
						double sec = rt.split();
						double fps = all_fields.size() / sec;
						cerr << "== search complete " << sec << " seconds (" << fps << " fields per second) ==" << endl;
					}
				}
			}
			search_cells = expand_cells(search_cells,s2cellid);

		}

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
