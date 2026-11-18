#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "arguments.hpp"
#include "field.hpp"
#include "point.hpp"
#include "cell_factory.hpp"

using namespace std;
using namespace silicontrip;

vector<string> split(const std::string& str, const std::string& delim) {
    vector<string> tokens;
    size_t start = 0;
    size_t end = str.find(delim);

    while (end != string::npos) {
        tokens.push_back(str.substr(start, end - start));
        start = end + delim.length();
        end = str.find(delim, start);
    }
    tokens.push_back(str.substr(start)); // Add the final token
    return tokens;
}

int main(int argc, char* argv[]) {

	arguments ag(argc,argv);


        if (!ag.parse())
        {
                cerr << "mucomp <field export>" << endl;
                exit(1);
        }

        if (ag.argument_size() < 1)
        {
                cerr << "mucomp <field export>" << endl;
                exit(1);
        }

	string filename = ag.get_argument_at(0);

	ifstream file(filename);

	if (!file.is_open()) {
		cerr << "Error: Could not open the file.\n";
		exit(1);
	}

/* 
OverlordMash |silicontrip |1052 |7261a047d55440b3ad492892db6ab644.b |346259922 |E |23dd585ed3494e83bf0added5d0b0fe3.16 |-35235098 |149048154 |3a7a6a3b3df0499190058f91c8c514a6.16 |-35228188 |149061337 |9f2bfdb8bb8a445cba3b0f65b6af2723.16 |-35208922 |149074017 |true

creator
reporting agent
mu
fguid
time
faction
p0guid
p0e6lat
p0e6lng
p1guid
p1e6lat
p1e6lng
p1guid
p1e6lat
p1e6lng
valid
*/


	cell_factory* cf = cell_factory::get_instance();
	string line;
	while (getline(file, line)) {
		vector<string>e = split(line," |");
		if (e.size() == 16)
		{
			int mu = stoi(e[2]);
			long p0e6lat = stol(e[7]);
			long p0e6lng = stol(e[8]);
			long p1e6lat = stol(e[10]);
			long p1e6lng = stol(e[11]);
			long p2e6lat = stol(e[13]);
			long p2e6lng = stol(e[14]);
			bool valid = e[15] == "true";

			point p0(p0e6lat,p0e6lng);
			point p1(p1e6lat,p1e6lng);
			point p2(p2e6lat,p2e6lng);

			field f(p0,p1,p2);

			cout << line << endl;

			int cmu = cf->get_mu(f);

			cout << "cmu:" << cmu << " mu:" << mu << endl;


			//cout << "INVALID: " << "mu:" << mu << " p0e6lat:" << p0e6lat << " p0e6lng:" << p0e6lng << " p1e6lat:" << p1e6lat << " p1e6lng:" << p1e6lng << " p2e6lat:" << p2e6lat << " p2e6lng:" << p2e6lng << " valid:" << valid << endl;
		}
	}

	file.close();

    }
