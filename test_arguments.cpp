#include <iostream>
#include <chrono>
#include <thread>
#include "run_timer.hpp"
#include "arguments.hpp"
#include "team_count.hpp"

using namespace std;

int main (int argc, char* argv[])
{
	using namespace silicontrip;
	run_timer rt;

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
	
	rt.start();
	cout << "start." << endl;	

	if (!ag.parse())
	{
		cout << "Options:" << endl;
		cout << " -R <number>       Limit number of Resistance Blockers" << endl;
		cout << " -C <#colour>      Set Drawtools output colour" << endl;
		cout << " -L                Set Drawtools to output as polylines" << endl;
		cout << " -O                Output as Intel Link" << endl;
		cout << " -M                Use MU calculation" << endl;
		cout << " -t <number>       Threshold for similar fields (larger less similar)" << endl;
		cout << " -l <number>       Maximum number of layers in plan" << endl;
		cout << " -p <percentile>   Use longest percentile links" << endl;
		cout << " -f <percentile>   Use largest percentile fields" << endl;
		cout << " -T <lat,lng,...>  Use only fields covering target points" << endl;
		exit(1);
	}

	cout << "split: " << rt.split() << endl;

	team_count tc = team_count(ag.get_option_for_key("E"),ag.get_option_for_key("R"),ag.get_option_for_key("N"));

	cout << tc << endl;

	if (ag.has_option("C"))
		cout << "set colour: " << ag.get_option_for_key("C") << endl;

	if (ag.has_option("l"))
		cout << "max layers: " << ag.get_option_for_key_as_int("l") << endl;

	cout << "stop: " << rt.stop() << endl;

	return 0;
}
