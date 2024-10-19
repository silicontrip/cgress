#include <iostream>
#include <chrono>
#include <thread>
#include <unordered_map>
#include "run_timer.hpp"
#include "portal_factory.hpp"
#include "link_factory.hpp"

using namespace std;

int main ()

{
	using namespace silicontrip;
	run_timer rt;

	srand(time(NULL));
	rt.start();

	cout << "start." << endl;	


	portal_factory* pf = portal_factory::get_instance();

	portal po = pf->get_single("The Peoples Garden");

	cout << po << endl;
	cout << "single " << rt.split() <<  endl;

	unordered_map<string,portal>* li = pf->cluster_from_file("./test_list.txt");

	for (auto it: *li) {
		cout << it.second << endl;
	}
	cout << "list " << rt.split() << endl;

	unordered_map<string,portal>* li2 = pf->cluster_from_description("The Peoples Garden:1");
	vector<portal>* poli = new vector<portal>();
	for (auto it: *li2) {
		cout << it.second << endl;
		poli->push_back(it.second);
	}

	cout << "read " << to_string(li2->size())<<" portals in " << rt.split() << " seconds." << endl;
	cout << endl;

	link_factory* lf = link_factory::get_instance();

	unordered_map<string,silicontrip::link>* li3 = lf->get_all_links();
	cout << "read: " + to_string(li3->size()) + " links in " << rt.split() <<" seconds." << endl;

	vector<silicontrip::link>*li4 = lf->purge_links(poli,li3);
	cout << "purged: " + to_string(li4->size()) + " links in " << rt.split() <<" seconds." << endl;

	vector<line>* li5 = lf->make_lines_from_single_cluster(poli);

	cout << "generated: " << li5->size() << " links in " << rt.split() << " seconds." << endl;

	vector<line>* li6 = lf->percentile_lines(li5,1);

	for (line line6: *li6)
	{
		cout << line6 << endl;
	}
	cout << "percentiled: " << li6->size() << " links in " << rt.split() << " seconds." << endl;

	team_count tc = team_count(0,0,0);

	vector<line>* li7 = lf->filter_links(li5,li4,tc);

	cout << "blocked: " << li7->size() << " links in " << rt.split() << " seconds." << endl;


	cout << "stop: " << rt.stop() << endl;

	return 0;
}
