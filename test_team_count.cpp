#include "team_count.hpp"
#include "run_timer.hpp"

#include<string>

using namespace std;

int main (int argc, char* argv[])
{
	using namespace silicontrip;
	run_timer rt;

	rt.start();

	cout << "start." << endl;	

	team_count want= team_count("0","","");

	cout << "want: " << want << endl;
	cout << rt.split() << endl;

	team_count have1 = team_count(1,2,0);
	cout << "have1: " << have1 << endl;
	team_count have2 = team_count("","1","");
	cout << "have2: " << have2 << endl;
	team_count have3 = team_count("","","1");
	cout << "have3: " << have3 << endl;

	cout << (have1 > want) << " ";
	cout << (have2 > want) << " ";
	cout << (have3 > want) << " ";

	cout << "Stop. " << rt.stop() << endl;


}
