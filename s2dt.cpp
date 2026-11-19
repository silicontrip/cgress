

#include "cell_factory.hpp"
#include "arguments.hpp"

using namespace silicontrip;
using namespace std;

int main (int argc, char* argv[])
{

	arguments ag(argc,argv);

	ag.parse();

	S2CellId id = S2CellId::FromToken(ag.get_argument_at(0));
	draw_tools dt = cell_factory::get_instance()->dt_cell(id);

	cout << dt.to_string() << endl;

	return 0;
}
