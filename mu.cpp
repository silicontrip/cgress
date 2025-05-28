#include "draw_tools.hpp"
#include "arguments.hpp"
#include "field_factory.hpp"

using namespace std;
using namespace silicontrip;


void print_usage()
{
		cerr << "Usage:" << endl;
		cerr << "mu <draw tools json> " << endl;
}

int main (int argc, char* argv[])
{
    arguments ag(argc,argv);

    if(!ag.parse())
    {
        print_usage();
        exit(1);
    } 

	if (ag.argument_size() != 1)
	{
		cerr << "Must specify drawtools plan" << endl;
		exit(1);
	}

	draw_tools dtp = draw_tools(ag.get_argument_at(0));

    vector<field> fields = dtp.get_fields();

    field_factory* ff = field_factory::get_instance();

    uniform_distribution total(0,0);
    for (field f : fields)
    {
        unordered_map<string,double> intersections = ff->cell_intersection(f);
    	vector<string> cells = ff->celltokens(f);
    	unordered_map<string,uniform_distribution> cellmu = ff->query_mu(cells);

        uniform_distribution ftotal(0,0);

        for (string ctok : cells)
        {
            uniform_distribution tud = cellmu[ctok] * intersections[ctok];
            ftotal = ftotal + tud;
            cout << ctok << " " << cellmu[ctok] << " x " << intersections[ctok] << " = " << tud << endl;
        }
        cout << ftotal << endl;
        total = total + ftotal;

    }
        cout << total << endl;

}