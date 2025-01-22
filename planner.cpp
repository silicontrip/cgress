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
	bool allow2km;
	vector<line> poly_lines;
	run_timer rt;

	random_device ran_dev;
	default_random_engine rng;
	mt19937 gen;

	unordered_map<field, vector<point>> polygons_map;

	vector<field>complete_field(const vector<line>&order, line pl);
	bool check_plan(const vector<line>&dts);
	bool check_same(const vector<line>&dts);
	
	double geo_cost(const vector<point>&pp);
	int count_visited(point pp, const vector<point>&visited);
	int count_unvisited(point pp, const vector<point>&visited);

	int key_cost(const vector<point>&order);

	double total_cost(const vector<point>&comb);
	static int factorial(int n);
	static bool next_permutation(vector<point>&visited, int step);

	draw_tools plan(draw_tools dts, vector<point>combination);
	vector<point> gen_random(vector<point>points);
	vector<point> perturb_solution(vector<point>points);

public:

	planner (double k, int s, draw_tools d, vector<point> din, vector<line>p, bool a2k);
	void simulated_annealing (vector<point> combination, double initial_temperature, double cooling_rate, int iterations);

};


	planner::planner (double k, int s, draw_tools d, vector<point> din, vector<line>p, bool a2k)
	{
		key_percent = k;
		sbul_available = s;
		dt = d;
		poly_lines = p;
		sbul_limit = 8 + sbul_available * 8;
		allow2km = a2k;

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

vector<field>planner::complete_field(const vector<line>&order, line pl)
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

bool planner::check_same(const vector<line>&dts)
{
	vector<line> order;

	for (int i=0; i < dts.size(); i++)
	{
		line pl = dts[i];
		vector<field> complete_fields = complete_field(order,pl);

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
			return 1000;
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

bool planner::next_permutation(vector<point>&visited, int step)
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

	for (int i = 1; i < combination.size(); i++) 
	{
		point this_point = combination[i];
		vector<point> out_links;

		out_links.erase(out_links.begin(),out_links.end());
		
		for (point visit_point : visited)
		{
			for (line po : poly_lines)
			{
				if ((this_point == po.get_o_point() && visit_point == po.get_d_point()) ||
					(this_point == po.get_d_point() && visit_point == po.get_o_point())) 
				{
					out_links.push_back(visit_point);
				}
			}
		}

		if (out_links.size() > 0)
		{	
			bool valid_plan = false;
			int counter = 0;

			int count_limit = factorial(out_links.size());

			vector<line> ndt;
			vector<line>new_line;

			while (!valid_plan && counter < count_limit) {
				ndt.assign(ldts.begin(),ldts.end());
				new_line.erase(new_line.begin(),new_line.end());

				next_permutation(out_links, counter++);

				for (point pp: out_links)
				{
					// not sure why but the definition is line(destination, origin)
					//line nline = line(this_point,pp);
					line nline = line(pp,this_point);

					ndt.push_back(nline);
					new_line.push_back(nline);
				}

				valid_plan = check_same(ndt);
			}

			for (line l : new_line)
				dts.add(l);
			ldts.assign(ndt.begin(),ndt.end());

		} else {
			dts.add(this_point);
		}
		visited.push_back(this_point);
	}

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

					cout <<  kcost << " "  << dist <<  " : (" << temperature << "/" << iter << ") " << rt.split() << " seconds" << endl << endl;
					if (kcost < 1000)
					{
						dt = plan(dt,best_combination);

						cout << dt << endl;
						cerr << rt.split() << " seconds" << endl << endl;

					}
				}
			}
		}
	}
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
		cerr << " -t <number>       Initial temperature default 2.0 (try single digit)" << endl;
		cerr << " -i <number>       Number of iterations default 2000 (try mutiple 1000s)" << endl;
		cerr << " -C <#colour>      Set Drawtools output colour" << endl;
}

int main (int argc, char* argv[])
{
	run_timer rt;

	double cost_percentage = 50.0;
	int sbul_count = 0;
	bool allow2km = true;
	double initial_temperature = 2.0;
	int iterations = 2000;

	arguments ag(argc,argv);

	ag.add_req("k","key",true);
	ag.add_req("s","sbul",true);
	ag.add_req("l","disallow2k",false);
	ag.add_req("t","temperature",true);
	ag.add_req("i","iterations",true);
	ag.add_req("C","colour",true);

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

	if (ag.argument_size() != 1)
	{
		cerr << "Must specify drawtools plan" << endl;
		exit(1);
	}

	draw_tools dtp = draw_tools(ag.get_argument_at(0));

	vector<line> poly_lines = dtp.get_lines();
	vector<point> combination = dtp.get_points();

	//	planner::planner (int k, int s, draw_tools d, vector<point> din, vector<line>p, bool a2k)

	planner p =  planner(cost_percentage, sbul_count, dt, combination, poly_lines, allow2km);

	p.simulated_annealing(combination, initial_temperature, 0.95, iterations);


}
