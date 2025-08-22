#include "draw_tools.hpp"
#include "run_timer.hpp"
#include "arguments.hpp"

#include <algorithm>
#include <random>

using namespace std;
using namespace silicontrip;

class agentassign {

private:
	draw_tools dt;

	vector<line>poly_lines;
	vector<int>agent_assign;
	vector<array<int,3>>field_lines;

	run_timer rt;

	random_device ran_dev;
	default_random_engine rng;
	mt19937 gen;

	int score(vector<int>assign);	
	vector<int> perturb_solution(vector<int>its);
	draw_tools plan(vector<int>assign);

public:

	agentassign(vector<line>pl, vector<int>aa, vector<array<int,3>>fl);
	void simulated_annealing (vector<int>combination, double initial_temperature, double cooling_rate, int iterations);

};

agentassign::agentassign(vector<line>pl, vector<int>aa, vector<array<int,3>>fl)
{
	poly_lines = pl;
	agent_assign = aa;
	field_lines = fl;
}

int agentassign::score(vector<int>assign)
{
	int total = 0;
	for (array<int,3> ft : field_lines)
	{
		int score = 0;
		if (assign[ft[0]]!=assign[ft[1]] && assign[ft[1]]!=assign[ft[2]] && assign[ft[2]]!=assign[ft[0]])
		{
			score = 2;
		} else if (assign[ft[0]]!=assign[ft[1]] || assign[ft[1]]!=assign[ft[2]] || assign[ft[2]]!=assign[ft[0]]) {
			score = 1;
		}
		total += score;
	}
	return total;
}

draw_tools agentassign::plan(vector<int>assign)
{

	draw_tools dt;
	for (int i=0; i < assign.size(); i++)
	{
		int agent = assign[i];
		dt.set_colour(agent);
		dt.add(poly_lines[i]);
	}
	return dt;
}

vector<int> agentassign::perturb_solution(vector<int>its)
{
	int n = its.size();

	uniform_int_distribution<> rand1(0, n-1);
	uniform_int_distribution<> rand2(0, n-2);

	int i = rand1(gen);
	int j = rand2(gen);
	if (j >= i) 
		j++;

	// cerr << "planner::perturb_solution " << i << " <-> " << j << endl;

	swap(its[i], its[j]);

	return its;
}

void agentassign::simulated_annealing (vector<int> assign, double initial_temperature, double cooling_rate, int iterations)
{
	int best_score = 0;
	rt.start();
	uniform_real_distribution<float> rand(0,1);

	vector<int> best_assign; 

	for (double temperature = initial_temperature; temperature > 1e-6; temperature *= cooling_rate)
	{

		int current_score = score(assign);

		for (int iter =0; iter < iterations; iter++)
		{
			
			//cerr << endl << "iter: " << iter <<  " " << rt.split() << " seconds" << endl;
			vector<int> new_assign = perturb_solution(assign);

			int new_score = score(new_assign);

			if (new_score > current_score || exp((new_score - current_score) / temperature) > rand(gen))
			{

				assign = new_assign;
				current_score = new_score;

				if (new_score > best_score)
				{
					best_score = new_score;
					best_assign = new_assign;

					cout <<  best_score << " : (" << temperature << "/" << iter << ") " << rt.split() << " seconds" << endl;
					draw_tools dt = plan(best_assign);
					cout << dt << endl << endl;
				}
			}
		}
	}
	//cout << "Plan fields: " << polygons_map.size() << " Links: " << poly_lines.size() << " Portals: " << best_combination.size() << ". " << rt.stop() << " seconds" << endl;
}


void print_usage()
{
	cerr << "Usage:" << endl;
	cerr << "agentassign [options] <draw tools json> " << endl;
	cerr << "Options:" << endl;
	cerr << " -2                Plan for 2 agents (default 3)" << endl;
	cerr << " -t <number>       Initial temperature default 2.0 (try single digit)" << endl;
	cerr << " -i <number>       Number of iterations default 2000 (try mutiple 1000s)" << endl;
}


int main (int argc, char* argv[])
{

	run_timer rt;
	int agents = 3;

	vector<int>assign;
	unordered_map<line,int> line_to_index;
	vector<array<int,3>>field_lines;
	double initial_temperature = 2.0;
	int iterations = 2000;

	arguments ag(argc,argv);

	ag.add_req("2","two",false); // two agents
	ag.add_req("t","temperature",true);
	ag.add_req("i","iterations",true);

	if (!ag.parse() || ag.has_option("h"))
	{
		print_usage();
		exit(1);
	}

	if (ag.has_option("t"))
		initial_temperature = ag.get_option_for_key_as_double("t");

	if (ag.has_option("i"))
		iterations = ag.get_option_for_key_as_int("i");

	if (ag.has_option("2"))
		agents = 2;

	if (ag.argument_size() != 1)
	{
		cerr << "Must specify drawtools plan" << endl;
		exit(1);
	}	

	draw_tools dtp = draw_tools(ag.get_argument_at(0));

	vector<line> poly_lines = dtp.get_lines();

	if (poly_lines.size() % agents) 
		cerr << "WARNING: links not divisible by agents. An imbalance will be present." << endl;
	
	for (int i = 0; i < poly_lines.size(); i++)
	{
		line_to_index[poly_lines[i]] = i;
		assign.push_back(i % agents);
	}

	// convert fields into link index clusters	
	
	for (field f : dtp.get_fields())
	{
		array<int,3> index_triplet;
		for (int i=0; i<3; i++)
		{
			int index = line_to_index[f.line_at(i)];
			index_triplet[i] = index;
		}
		field_lines.push_back(index_triplet);
	}

	agentassign aa(poly_lines, assign, field_lines);

	aa.simulated_annealing(assign,initial_temperature, 0.95, iterations);
	
	
}
