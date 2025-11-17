#include <iomanip>
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
	uniform_distribution current_mu;
	double epsilon = 1e-6;

	string draw_fields(const vector<field>& f);
	double calc_score(const field& f) const;
	double search_fields(vector<field> current, const field& f, int start, double best);
	vector<vector<uniform_distribution> > multi_ranges(const vector<field>& vf) const;

	uniform_distribution lowest(const vector<vector<uniform_distribution> >& r, uniform_distribution c, size_t index) const;
	vector<int> decomb(const vector<int>& outcomes_per_field, int current_index) const;
	uniform_distribution opt_lowest(const vector<vector<uniform_distribution> >& r,uniform_distribution c) const;

	double multi_improvement(const vector<field>& vf, const field& fi) const;
	double multi_improvement(const vector<field>& vf) const;
	bool compare_improvement(const field& f1, const field& f2) const;
	vector<field> new_fields(vector<field>& current, const field& f) const;
	double pimprovement(const field& f) const;
	vector<uniform_distribution> ranges(const field& f1, uniform_distribution mucell) const;
	pair<int,int> range(const field& f1, double area, uniform_distribution mucell, uniform_distribution othermu) const;


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

	vector<string>cells;
	cells.push_back(tok);
	unordered_map<string,uniform_distribution> cellmu = ff->query_mu(cells);
	current_mu = cellmu[tok];

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

uniform_distribution other_contribution(const field& f, string celltok, const unordered_map<string,double>& intersections,const unordered_map<string,uniform_distribution>& cellmu)
{

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

pair<int,int> cellfields::range(const field& f1, double area, uniform_distribution mucell, uniform_distribution othermu) const
{

	//uniform_distribution othermu1 = other_contribution(f1,cell_token,intersections1,cellmu1);
	uniform_distribution totalmu1 = othermu + mucell * area;

	double totalmax1 = round(totalmu1.get_upper());
	double totalmin1 = round(totalmu1.get_lower());

    if (totalmin1==0)
        totalmin1=1;

    if (totalmax1==0)
        totalmax1=1;

	pair<int,int> res(totalmin1,totalmax1);

	return res;
}

vector<uniform_distribution> cellfields::ranges(const field& f1, uniform_distribution mucell) const
{
	unordered_map<string,double> intersections1 = ff->cell_intersection(f1);
	vector<string> cells1 = ff->celltokens(f1);
	unordered_map<string,uniform_distribution> cellmu1 = ff->query_mu(cells1);

	uniform_distribution othermu1 = other_contribution(f1,cell_token,intersections1,cellmu1);
	pair<int,int> murange = range(f1,intersections1[cell_token], mucell,othermu1);

	vector<uniform_distribution> res;
	for (int tmu1 = murange.first; tmu1 <= murange.second; tmu1++)
    {
        uniform_distribution mu1(tmu1-0.5,tmu1+0.5);
        if (tmu1==1)
            mu1 = uniform_distribution(0.0,1.5);

        uniform_distribution remain1 = (mu1 - othermu1) / intersections1[cell_token]; //remaining(mu, f, celltok) / intersections[celltok];
        uniform_distribution intremain1 = remain1.intersection(mucell);
		res.push_back(intremain1);
	}
	return res;
}

uniform_distribution cellfields::lowest(const vector<vector<uniform_distribution> >& r, uniform_distribution c, size_t index) const
{

	if (index >= r.size())
		return c;

	uniform_distribution worst (0.0,0.0);
	for (uniform_distribution u : r[index])
	{

		uniform_distribution t = c.intersection(u);
		if (t.range() == 0)
			t = c;

		uniform_distribution next_rd;
		if (index +1 < r.size())
			next_rd = lowest(r,t,index+1);
		else
			next_rd = t;

		uniform_distribution id = c.intersection(next_rd);
		if (id.range() == 0)
			id = c;
		if (id.range() > worst.range())
			worst = id;
	}
	return worst;
}

vector<int> cellfields::decomb(const vector<int>& outcomes_per_field, int current_index) const
{
    vector<int> result_indices(outcomes_per_field.size());

    // Iterate from the last field to the first (or first to last, depending on convention)
    // Let's assume we fill result_indices from right to left (least significant "digit" first)
    for (int i = outcomes_per_field.size() - 1; i >= 0; --i) {
        result_indices[i] = current_index % outcomes_per_field[i];
        current_index /= outcomes_per_field[i];
    }
    return result_indices;
}

uniform_distribution cellfields::opt_lowest(const vector<vector<uniform_distribution> >& r,uniform_distribution c) const
{
    vector<int> fval;
    int all = 1;
    int field_length = r.size();
    for (vector<uniform_distribution> vud : r)
    {
        all *= vud.size();
        fval.push_back(vud.size());
    }

    uniform_distribution worst(0.0,0.0);
    for (int cc =0; cc< all; cc++)
    {
        uniform_distribution itworst = c;
        vector<int>current = decomb(fval,cc);
        for(int icc=0; icc<field_length; icc++)
        {
            uniform_distribution field_int = r[icc][current[icc]];
            itworst = itworst.intersection(field_int);
        }
        // not sure if I can directly compare them
        if (itworst.range() >= c.range())
            return c;
        if (itworst.range() > worst.range())
            worst = itworst;
    }
    return worst;
}

vector<vector<uniform_distribution> > cellfields::multi_ranges(const vector<field>& vf) const
{
	vector<vector<uniform_distribution> > existing;
	if (vf.size() > 0)
	{
		for (int j =0; j < vf.size(); j++)
		{
			field f = vf[j];
			vector<uniform_distribution> fd = ranges(f,current_mu);
			existing.push_back(fd);
		}
	}
	return existing;
}

double cellfields::multi_improvement(const vector<field>& vf, const field& fi) const
{
	vector<vector<uniform_distribution> > existing = multi_ranges(vf);

	vector<uniform_distribution> fd = ranges(fi,current_mu);
	existing.push_back(fd);

	uniform_distribution best = opt_lowest(existing,current_mu);

	return current_mu.range() / best.range();
}

double cellfields::multi_improvement(const vector<field>& vf) const
{
	vector<vector<uniform_distribution>> existing = multi_ranges(vf);
	uniform_distribution best = opt_lowest(existing,current_mu);
	return current_mu.range() / best.range();
}

double cellfields::pimprovement(const field& f) const
{

	vector<uniform_distribution> field_improvements = ranges(f,current_mu);

	double current_range = current_mu.range();
	double worst = 0.0;
    for (uniform_distribution intremain : field_improvements)
    {
		double intremainrange = intremain.range();

		if (intremainrange+epsilon >= current_range) // might need to epsilon this
			return 1.0;
        if (intremainrange > worst)
            worst = intremainrange;

        // cerr << "mu: " << tmu << " rem: " << remain << " range: " << intremain.range() << " imp: " << cellmu[celltok].range() / intremain.range() <<   endl;

    }
	return current_range / worst;

}

vector<field> cellfields::new_fields(vector<field>& current, const field& f) const
{
	vector<field> start (current);
	vector<field> result;
	start.push_back(f);
	double original_score = multi_improvement(start);
	for (size_t i=0; i < start.size(); i++)
	{
		vector<field> temp;
		if (i>0)
			temp.insert(temp.end(),start.begin(),start.begin()+i-1);
		if (i<start.size()-1)
			temp.insert(temp.end(),start.begin()+i+1,start.end());

		double test = multi_improvement(temp);

		if (test < original_score)
			result.push_back(start[i]);

	}
	return result;
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

	double fscore = pimprovement(f);
	//double fscore = multi_improvement(f);
	//pimprovement(f);

	if (fscore <= 1.0 + epsilon)
		return best;

	double total_score=fscore;
	if (current.size() > 0)
	{
		total_score = multi_improvement(current,f);  // still not working properly. This function needs to be recursive.

		// if (best >= total_score)
		//	return best;
	}

	//current = new_fields(current,f);

//	int newSize = current.size();


	//cerr << "fscore: " << setprecision(20) << fscore << endl;
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

	if (total_score > best)
	{
		vector<field> temp_plan = new_fields(current,f);
		int newSize = temp_plan.size();
		if (limit_layers==0 || newSize <= limit_layers)
		{
			cerr << total_score << " : " << newSize << " : " << rt.split() << " seconds." << endl;
			cout << draw_fields(temp_plan) << endl;
			cerr << endl;
			best = total_score;
		}
	}
	current.push_back(f);

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
    return a.geo_perimeter() < b.geo_perimeter();
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
		cerr << " -i <cluster>      Ignore blocking links from these portals" << endl;

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

vector<line> filter_lines (const vector<line>& li, const vector<silicontrip::link>& links, const team_count& tc, const vector<portal>& avoid_double, bool limit2k)
{
	link_factory* lf = link_factory::get_instance();
    vector<line> la = lf->filter_links(li, links, tc);

	if (avoid_double.size() > 0)
        la = lf->filter_link_by_blocker(la, links, avoid_double);

	if (limit2k)
        la = lf->filter_link_by_length(la, 2);

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
	vector<portal>avoid_double;
	vector<portal>ignore_links;

	string cellid;
	int limit=0;

	bool limit2k = false;

	arguments ag(argc,argv);

	ag.add_req("E","enlightened",true); // max enlightened blockers
	ag.add_req("R","resistance",true); // max resistance blockers
	ag.add_req("N","machina",true); // max machina blockers
	ag.add_req("S","avoid", true); // avoid using these portals.
	ag.add_req("i","ignore",true); // ignore links from these portals (about to decay or easy to destroy)
	ag.add_req("D","blockers",true); // remove links with blocker using these portals.


	ag.add_req("C","colour",true); // drawtools colour
	ag.add_req("I","intel",false); // output as intel
	ag.add_req("L","polyline",false); // output as polylines
	ag.add_req("l","limit",true); // limit layers/fields
	ag.add_req("k","limit2k",false); // limit link length to that can be made under fields.
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

	if (ag.has_option("k"))
		limit2k=true;



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
	{
		cerr << "== Reading Avoid Portals ==" << endl;
		avoid_single = pf->cluster_from_description(ag.get_option_for_key("S"));
	}
	if (ag.has_option("i"))
	{
		cerr << "== Reading Ignore Links Portals ==" << endl;
		ignore_links = pf->cluster_from_description(ag.get_option_for_key("i"));
	}

	if (ag.has_option("D"))
	{
    	cerr << "== Reading Avoid Portals ==" << endl;
		avoid_double = pf->cluster_from_description(ag.get_option_for_key("D"));
		// for (portal p: avoid_double)
		//	cerr << "avoid: " << p << endl;
	}

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
			cerr << "search iteration: " << iteration++ << ". " << search_cells.size() << " cells." << endl;
			stringstream cluster_desc;

			//cluster_desc << cell_centre.lat() << "," << cell_centre.lng() << ":" << range;

			//cerr << "Query: " << cluster_desc.str() << endl;

			vector<portal> portals = cluster_and_filter_from_cell_set(avoid_single, search_cells);

			cerr << "== " << portals.size() << " portals read. in " << rt.split() << " seconds. ==" << endl;
			if (portals.size() > 2)
			{
				//cerr << "== getting links ==" << endl;

				links = lf->get_purged_links(portals);
				if (!ignore_links.empty())
					links = lf->filter_link_by_portal(links,ignore_links);
				//cerr <<  "== " << links.size() << " links read. in " << rt.split() <<  " seconds ==" << endl;

				//cerr << "== generating potential links ==" << endl;

				vector<line> li = lf->make_lines_from_single_cluster(portals);
				//cerr << "all links: " << li.size() << endl;
		        li = filter_lines(li, links, tc, avoid_double, limit2k);

				//li = lf->filter_links(li,links,tc);
				//if (limit2k)
        		//	li = lf->filter_link_by_length(li, 2);
				//cerr << "purged links: " << li.size() << endl;
				cerr << "==  "  << li.size() << " links generated " << rt.split() <<  " seconds ==" << endl;
				if (li.size() > 2)
				{
					//cerr << "== Generating fields ==" << endl;

					all_fields = ff->make_fields_from_single_links_v2(li);
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
