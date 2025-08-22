#include "draw_tools.hpp"
#include "run_timer.hpp"
#include "arguments.hpp"

#include <algorithm>
#include <random>

using namespace std;
using namespace silicontrip;


class planner {

private:
	double key_percent;
	int sbul_available;
	draw_tools dt;
	int sbul_limit;
	bool outbound_message_shown;
	bool allow2km;
	bool reverse_plan;
	vector<line> poly_lines;
	run_timer rt;

	random_device ran_dev;
	default_random_engine rng;
	mt19937 gen;

	unordered_map<field, vector<point>> polygons_map;

	vector<field>complete_field(const vector<line>&order, const line& pl);
	bool check_plan(const vector<line>&dts);
	bool check_same(const vector<line>&dts);
	bool check_same(vector<line>dts,const vector<line>& ndts);
	vector<int> find_lines(const vector<field>& f, const vector<line>& ndts, const line& current);
	vector<line> make_valid(vector<line>dts,vector<line> ndts);
	unordered_map<field,int> count_fields(vector<field>f);
	vector<line> remove_lines(field f, vector<line> l1, vector<line> l2);


	double geo_cost(const vector<point>&pp);
	int count_visited(point pp, const vector<point>&visited);
	int count_unvisited(point pp, const vector<point>&visited);

	int key_cost(const vector<point>&order);

	double total_cost(const vector<point>&comb);
	static int factorial(int n);
	static bool next_permutation(vector<line>&visited, int step);

	draw_tools plan(draw_tools dts, vector<point>combination);
	vector<point> gen_random(vector<point>points);
	vector<point> perturb_solution(vector<point>points);

public:

	planner (double k, int s, draw_tools d, vector<point> din, vector<line>p, bool a2k, bool r);
	void simulated_annealing (vector<point> combination, double initial_temperature, double cooling_rate, int iterations);

};

planner::planner (double k, int s, draw_tools d, vector<point> din, vector<line>p, bool a2k, bool r)
{
	key_percent = k;
	sbul_available = s;
	dt = d;
	poly_lines = p;
	sbul_limit = 8 + sbul_available * 8;
	outbound_message_shown = false;
	allow2km = a2k;
	reverse_plan = r;

	vector<line> order;

	for (int i = 0; i < p.size(); i++) {
		line pl = p[i];
		vector<field> complete_fields = complete_field(order, pl);
		for (field pg : complete_fields) {
			if (polygons_map.find(pg) == polygons_map.end())
			{
				vector<point> vp;
				polygons_map[pg] = vp;
			}
			for (point pp: din)
			{
				if (pg.inside(pp))
				{
					polygons_map[pg].push_back(pp);
				}
			}
		}
		order.push_back(pl);
	}

}

bool planner::check_plan(const vector<line>& dt_lines)
{
	//vector<line> dt_lines = dts.get_lines();
	vector<line> order;
	unordered_set<point> covered_points;

	
	for (int i=0; i < dt_lines.size(); i++)
	{
		line pl = dt_lines[i];
		if (covered_points.find(pl.get_o_point()) != covered_points.end() && !(pl.geo_distance() <= 2.0 && allow2km))
			return false;
	
		vector<field> complete_fields = complete_field(order,pl);

		for (field pg: complete_fields) {
			if (polygons_map.find(pg) != polygons_map.end())
			{
				covered_points.insert(polygons_map[pg].begin(), polygons_map[pg].end());
			}
		}
		order.push_back(pl);
	}

	return true;
}

double planner::geo_cost(const vector<point>&pp)
{
	double distance = 0;

	for (int i=1; i<pp.size(); i++)
	{
		point o = pp.at(i-1);
		point d = pp.at(i);
		distance += o.geo_distance_to(d);
	}
	return distance;
}

int planner::count_visited(point pp, const vector<point>&visited)
{
	int cost = 0;
	for (line poly : poly_lines)
	{
		point p1 = poly.get_o_point();
		point p2 = poly.get_d_point();
		bool has_visited = false;

		if (pp == p1 || pp == p2)
		{
			for (point visit : visited) 
			{
				if (visit == p1 || visit == p2)
				{
					has_visited = true;
					break;
				}
			}
			if (has_visited)
				cost ++;
		}
	}
	return cost;
}

int planner::count_unvisited(point pp, const vector<point>&visited)
{
	int cost = 0;
	for (line poly : poly_lines)
	{
		point p1 = poly.get_o_point();
		point p2 = poly.get_d_point();
		bool has_visited = false;

		if (pp == p1 || pp == p2)
		{
			for (point visit : visited) 
			{
				if (visit == p1 || visit == p2)
				{
					has_visited = true;
					break;
				}
			}
			if (!has_visited)
				cost ++;
		}
	}
	return cost;
}

int planner::key_cost(const vector<point>&order)
{
	vector<point>visited;
	int max_cost = 0;
	for (int i=0; i<order.size(); i++)
	{
		int link_limit = count_visited(order.at(i), visited);
		// cerr << "link_limit " << link_limit << endl;
		if (link_limit>sbul_limit)
		{
			if (!outbound_message_shown)
			{
				cerr << "Outbound links exceeded. Consider SBUL." << endl;
				outbound_message_shown = true;
			}
			return 1000;
		}
		int point_cost = count_unvisited(order.at(i),visited);
		// cerr << "point_cost " << point_cost << endl; 
		if (point_cost > max_cost)
			max_cost = point_cost;
		visited.push_back(order.at(i));
	}
	//cerr << "max_cost " << max_cost << endl;
	return max_cost;
}

double planner::total_cost(const vector<point>&comb)
{
	vector<line> dts;
	int kcost = key_cost (comb);
	double dist = geo_cost (comb);

	if (kcost < 1000)
	{
		if (!check_plan(dts))
			kcost = 10000;
	}
	return (kcost * key_percent / 100.0 ) + ( dist * 1 - key_percent / 100.0);
}

int planner::factorial(int n) {
	int result = 1;
	for (int i = 2; i <= n; i++) {
		result *= i;
	}
	return result;
}

vector<field>planner::complete_field(const vector<line>&order, const line& pl)
{
	vector<field> completed_fields;

	for (int i = 0; i < order.size(); i++) {
		line p1 = order.at(i);

		for (int j = i + 1; j < order.size(); j++) {
			line p2 = order.at(j);

			point p1o = p1.get_o_point();
			point p1d = p1.get_d_point();
			point p2o = p2.get_o_point();
			point p2d = p2.get_d_point();
			point plo = pl.get_o_point();
			point pld = pl.get_d_point();

			if (p1o == p2o &&
				((plo ==p1d && pld == p2d) ||
				(plo == p2d  && pld == p1d))
			) {
				field potential_polygon = field(p1o,plo,pld);
				completed_fields.push_back(potential_polygon);
			} else if (p1d == p2o &&
				((plo == p1o  && pld == p2d) ||
				(plo == p2d && pld == p1o))
			) {
				field potential_polygon = field(p1d,plo,pld);
				completed_fields.push_back(potential_polygon);
			} else if (p1o == p2d &&
				((plo == p1d && pld == p2o) ||
				(plo == p2o && pld == p1d))
			) {
				field potential_polygon = field(p1o,plo,pld);
				completed_fields.push_back(potential_polygon);
			} else if (p1d == p2d &&
				((plo == p1o && pld == p2o) ||
				(plo == p2o  && pld == p1o))
			) {
				field potential_polygon =  field(p1d,plo,pld);
				completed_fields.push_back(potential_polygon);
			} 
		}
	}
	return completed_fields;
}

vector<int> planner::find_lines(const vector<field>& f, const vector<line>& ndts, const line& current)
{
	vector<int> found;
	for (int i=0; i < ndts.size(); ++i)
	{
		line li = ndts[i];
		if (!(current == li))
			for (field fi: f)
				if (fi.has_line(li))
					found.push_back(i);
	}			
	return found;
}

void out_debug(const vector<line>& l, const line current)
{
	draw_tools dt;

	for (line li: l)
		dt.add(li);

	dt.set_colour("#FFFFFF");
	dt.add(current);

	cerr << dt << endl;
}

unordered_map<field,int> planner::count_fields(vector<field>f)
{
	unordered_map<field,int> field_count;
	for (field f1: f)
	{
		int count = 0;
		for (field f2: f)
		{
			if (!(f1 == f2))
				if (f1.inside(f2))
					count ++;
		}
		field_count[f1] = count;
	}
	return field_count;
}

vector<line> planner::remove_lines(field f, vector<line> l1, vector<line> l2)
{
	vector<line> remain;
	for (line fl: f.get_lines())
	{
		bool found = false;
		for (line li: l1)
		{
			if (li == fl)
			{
				found = true;
				break;
			}
		}
		for (line li: l2)
		{
			if (li == fl)
			{
				found = true;
				break;
			}
		}
		if (!found)
			remain.push_back(fl);
	}
	return remain;
}

vector<line> planner::make_valid(vector<line>dts,vector<line> ndts)
{
	if (ndts.size() == 1)
		return ndts;

	vector<field> make_fields;
	vector<line>nndts = dts;
	for(line li: ndts)
	{
		vector<field> complete_fields = complete_field(nndts,li);
		make_fields.insert(make_fields.end(),complete_fields.begin(),complete_fields.end());
		nndts.push_back(li);
	}
	unordered_map<field,int> fc = count_fields(make_fields);
	vector<line> link_order;
	while (fc.size() > 0)
	{
		int max = -1;
		field max_field;
		for (pair<field,int>vv: fc) {
			if (vv.second > max)
			{
				max = vv.second;
				max_field = vv.first;
			}
		}
		vector<line>add_lines = remove_lines(max_field,link_order,dts);

		// lines need to be orientated the same way as the ndts array
		for (line la: add_lines)
			for (line ln: ndts)
				if (la == ln)
					link_order.push_back(ln);
		fc.erase (max_field);
	}

	// handle input links which aren't in the output...
	vector<line> unused_links;
	for (line ln: ndts)
	{
		bool found = false;
		for (line lo: link_order)
		{

			if (ln == lo)
			{
				found=true;
				break;
			}
		}
		if (!found)
			unused_links.push_back(ln);
	}
	unused_links.insert(unused_links.end(),link_order.begin(),link_order.end());
	return unused_links;
}

// I'm leaving this in for now incase there are flaws with my make_valid function
bool planner::check_same(vector<line>dts,const vector<line>& ndts)
{

	for (int i=0; i < ndts.size(); i++)
	{
		line pl = ndts[i];
		vector<field> complete_fields = complete_field(dts,pl);

		if (complete_fields.size() > 2)
			return false;

		if (complete_fields.size() == 2)
		{
			if (complete_fields[0].inside(complete_fields[1]))
			{
				return false;
			}
			if (complete_fields[1].inside(complete_fields[0]))
			{
				return false;
			}

		}
		dts.push_back(pl);
	}

	return true;
}

bool planner::next_permutation(vector<line>&visited, int step)
{
	const int n = visited.size();
        
	if (n==1)
		return true;
        // If step exceeds the total number of permutations, return false (end of permutations)
	if (step >= factorial(n)) {
		return false;
	}

	std::vector<int> p(n);

	// int p[n]; // = new int[n];

	for (int i=0; i < n; i++)
		p[i] = 0;

	int j = 1;
	int i = 1;
	if (step > 0)
		while (i < n)
		{
			if (p[i] < i)
			{
				j = i % 2 * p[i];
				step --;
				if (step == 0)
					break;
				p[i]++;
				i = 1;
			} else {
				p[i] = 0;
				i++;
			}
		}
	// Perform the swap with only one element per iteration based on swapIndex

	// cerr << "perm swap: " << i << " <-> " << j << endl;
	swap(visited[i], visited[j]);
	return true;
}

draw_tools planner::plan(draw_tools dts, vector<point>combination)
{
	dts.erase();
	vector<line>ldts;
	vector<point>visited; 
	visited.push_back(combination.at(0));
	dts.add(combination.at(0));
	run_timer drt;

	drt.start();
	for (int i = 1; i < combination.size(); i++) 
	{
		point this_point = combination[i];
		vector<line> out_links;

		out_links.erase(out_links.begin(),out_links.end());
		
		for (point visit_point : visited)
		{
			for (line po : poly_lines)
			{
				if ((this_point == po.get_o_point() && visit_point == po.get_d_point()) ||
					(this_point == po.get_d_point() && visit_point == po.get_o_point())) 
				{
					// not sure why but the definition is line(destination, origin)
					line nline = line(visit_point,this_point);
					out_links.push_back(nline);
				}
			}
		}

		if (out_links.size() > 0)
		{	
			//cerr << "out_links = " << out_links.size() << " : " << drt.split() << " seconds." << endl;
			/*
			bool valid_plan = false;
			int counter = 0;
			int count_limit = factorial(out_links.size());

			while (!valid_plan && counter < count_limit) {
				next_permutation(out_links, counter++);
				valid_plan = check_same(ldts,out_links);
			}
			if (!valid_plan)
				cerr << "No valid plans exist for this path." << endl;
			*/
 
			// I think I nailed the field heuristic logic rather than needing to test permutations as above.
 			out_links = make_valid(ldts,out_links);
 
			// There should be at least 1 valid order
			//cerr << "counter = " << counter << " : " << drt.split() << " seconds." << endl;

			for (line l : out_links)
			{
				ldts.push_back(l);
				dts.add(l);
			}

		} else {
			dts.add(this_point);
		}
		visited.push_back(this_point);
	}
	//cerr << "finish " << drt.stop() << " seconds." << endl;

	return dts;
}

vector<point> planner::gen_random(vector<point>points)
{
	shuffle(begin(points),end(points), rng);
	return points;
}

vector<point> planner::perturb_solution(vector<point>points)
{
	int n = points.size();

    uniform_int_distribution<> rand1(0, n-1);
    uniform_int_distribution<> rand2(0, n-2);

	int i = rand1(gen);
	int j = rand2(gen);
	if (j >= i) 
		j++;

	// cerr << "planner::perturb_solution " << i << " <-> " << j << endl;

	swap(points[i], points[j]);

	return points;
}

void planner::simulated_annealing (vector<point> combination, double initial_temperature, double cooling_rate, int iterations)
{
	double best_cost = 1000;
	rt.start();
	uniform_real_distribution<float> rand(0,1);

	// int n = combination.size();
	vector<point> best_combination; // = new ArrayList<>();
	vector<point> current_combination = gen_random(combination);

	for (double temperature = initial_temperature; temperature > 1e-6; temperature *= cooling_rate)
	{
		//ArrayList<PolyPoint> currentCombination = generateRandom(combination);

		double current_cost = total_cost(current_combination);

		for (int iter =0; iter < iterations; iter++)
		{
			
			//cerr << endl << "iter: " << iter <<  " " << rt.split() << " seconds" << endl;
			vector<point> new_combination = perturb_solution(current_combination);

			double new_cost = total_cost(new_combination);

			if (new_cost < current_cost || exp((current_cost - new_cost) / temperature) > rand(gen))
			{

				current_combination = new_combination;
				current_cost = new_cost;

				if (new_cost < best_cost)
				{
					best_cost = new_cost;
					best_combination = new_combination;

					int kcost = key_cost(best_combination);
					double dist = geo_cost(best_combination);

					cout <<  kcost << " "  << dist <<  " : (" << temperature << "/" << iter << ") " << rt.split() << " seconds" << endl;
					if (kcost < 1000)
					{
						if (reverse_plan)
						{
							vector<point> reverse_combination;
							reverse_combination.assign(best_combination.begin(),best_combination.end());
							reverse(reverse_combination.begin(), reverse_combination.end());
							dt = plan(dt,reverse_combination);
						} else {
							dt = plan(dt,best_combination);
						}

						cout << dt << endl << endl;
						// cerr << rt.split() << " seconds" << endl << endl;
						outbound_message_shown = false;

					}
				}
			}
		}
	}
	cout << "Plan fields: " << polygons_map.size() << " Links: " << poly_lines.size() << " Portals: " << best_combination.size() << ". " << rt.stop() << " seconds" << endl;
}

void print_usage()
{
		cerr << "Usage:" << endl;
		cerr << "planner [options] <draw tools json> " << endl;
		cerr << "Options:" << endl;
		cerr << " -k <number>       Percentage of key cost weight (default 50 == 500m)" << endl;
		cerr << "                   Lower value means favour shorter path but may require more keys." << endl;
		cerr << " -s <number>       Number of SBUL available (don't blame me if you specify more than 4 or 2 for a single agent)" << endl;
		cerr << " -l                Don't allow links less than 2km originating under fields" << endl;
		cerr << " -r                Print the reverse of the plan (may result in different key cost)" << endl;
		cerr << " -t <number>       Initial temperature default 2.0 (try single digit)" << endl;
		cerr << " -i <number>       Number of iterations default 2000 (try mutiple 1000s)" << endl;
		cerr << " -C <#colour>      Set Drawtools output colour" << endl;
		cerr << " -I                Output Drawtools as intel" << endl;
}

int main (int argc, char* argv[])
{
	run_timer rt;

	double cost_percentage = 50.0;
	int sbul_count = 0;
	bool allow2km = true;
	double initial_temperature = 2.0;
	int iterations = 2000;
	bool rev = false;

	arguments ag(argc,argv);

	ag.add_req("k","key",true);
	ag.add_req("s","sbul",true);
	ag.add_req("l","disallow2k",false);
	ag.add_req("t","temperature",true);
	ag.add_req("i","iterations",true);
	ag.add_req("C","colour",true);
	ag.add_req("I","intel",false);
	ag.add_req("r","reverse",false);

	if (!ag.parse() || ag.has_option("h"))
	{
		print_usage();
		exit(1);
	}

	draw_tools dt;

	if (ag.has_option("C")) {
		cerr << "set colour: " << ag.get_option_for_key("C") << endl;
		dt.set_colour(ag.get_option_for_key("C"));
	}

// this might defeat the purpose of this tool.
	if (ag.has_option("I"))
		dt.set_output_as_intel();

	if (ag.has_option("k"))
		cost_percentage = ag.get_option_for_key_as_double("k");

	if (ag.has_option("s"))
		sbul_count = ag.get_option_for_key_as_int("s");

	if (ag.has_option("l"))
		allow2km = false;

	if (ag.has_option("t"))
		initial_temperature = ag.get_option_for_key_as_double("t");

	if (ag.has_option("i"))
		iterations = ag.get_option_for_key_as_int("i");

	if (ag.has_option("r"))
		rev = true;

	if (ag.argument_size() != 1)
	{
		cerr << "Must specify drawtools plan" << endl;
		exit(1);
	}

	draw_tools dtp = draw_tools(ag.get_argument_at(0));

	vector<line> poly_lines = dtp.get_lines();
	vector<point> combination = dtp.get_points();

	//	planner::planner (int k, int s, draw_tools d, vector<point> din, vector<line>p, bool a2k)

	planner p(cost_percentage, sbul_count, dt, combination, poly_lines, allow2km, rev);

	p.simulated_annealing(combination, initial_temperature, 0.95, iterations);


}
