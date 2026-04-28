#include <iostream>
#include <chrono>
#include <thread>
#include <unordered_map>
#include "link_factory.hpp"
#include "run_timer.hpp"
#include "portal_factory.hpp"
#include "arguments.hpp"
#include "draw_tools.hpp"


using namespace std;

void print_usage()
{
		cerr << "Usage:" << endl;
		cerr << "portallist [options] <portal cluster> " << endl;
		cerr << "  Only 1 of L, P or I may be specified" << endl;
		cerr << "Options:" << endl;
		cerr << " -C|--colour <#colour>   Set Drawtools output colour" << endl;
		cerr << " -L                Set Drawtools to output as polylines" << endl;
		cerr << " -P                Set Drawtools to output as polygons" << endl;
		cerr << " -I                Output as Intel Link" << endl;
}

int main (int argc, char* argv[])
{
	using namespace silicontrip;
	run_timer rt;

	int output_type = 0;

	//srand(time(NULL));
	rt.start();

	arguments ag(argc,argv);
	ag.add_req("C","colour",true); // drawtools colour
	ag.add_req("h","help",false);
	ag.add_req("L","polyline",false);
	ag.add_req("P","polygon",false);
	ag.add_req("I","intel",false);


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

	if (ag.has_option("L"))
		output_type = 1;

	if (ag.has_option("P"))
	{
		if (output_type==0)
		{
			output_type = 2;
		} else {
			cerr << "Cannot specify P with L." << endl;
			print_usage();
			exit(1);
		}
	}

	if(ag.has_option("I"))
	{
		if (output_type==0)
		{
			output_type = 3;
		} else {
			cerr << "Cannot specify I with L or P." << endl;
			print_usage();
			exit(1);
		}
	}

	draw_tools dt;

	if (ag.has_option("C")) {
		cerr << "set colour: " << ag.get_option_for_key("C") << endl;
		dt.set_colour(ag.get_option_for_key("C"));
	}

	cerr << "start." << endl << endl;	

	try {
		portal_factory* pf = portal_factory::get_instance();
		link_factory* lf = link_factory::get_instance();

		vector<portal> all_portals;

		if (ag.argument_size() == 1)
			all_portals = pf->cluster_from_description(ag.get_argument_at(0));
	
    	vector<silicontrip::link> links = lf->get_purged_links(all_portals);

		for (silicontrip::link li : links)
			dt.add(li);

		if (output_type == 1)
			dt.set_output_as_polyline();
		if (output_type == 2)
			dt.set_output_as_polygon();
		if (output_type == 3)
			dt.set_output_as_intel();

		cout << dt.to_string() << endl;

	} catch (exception &e) {
		cerr << "An Error occured: " << e.what() << endl;
	}
	cerr << endl << "stop: " << rt.stop() << endl;

	return 0;
}
