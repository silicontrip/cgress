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

void predict(string celltok)
{
	vector<string> cells;
	cells.push_back(celltok);
	field_factory* ff = field_factory::get_instance();
	unordered_map<string,uniform_distribution> cellmu = ff->query_mu(cells);

	uniform_distribution this_mu =cellmu[celltok];
    uniform_distribution ilower = cellmu[celltok];
    uniform_distribution iupper = cellmu[celltok];
	for (double sz=0.0; sz < 1.0; sz += 0.0001)
	{
		uniform_distribution totalmu = this_mu * sz;
		double totalmax = round(totalmu.get_upper());
		double totalmin = round(totalmu.get_lower());
		if (totalmin > 1)
		{
			if (totalmax - totalmin == 1)
			{
				uniform_distribution mul(totalmin-0.5,totalmin+0.5);
				uniform_distribution muu(totalmax-0.5,totalmax+0.5);

                mul = mul / sz;
                muu = muu / sz;

                ilower = ilower.intersection(mul);
                iupper = iupper.intersection(muu);

                cerr << "size: " << sz << " mu lower: " << totalmin << " rem: " << mul << " " << ilower << " mu upper:  " << totalmax << " rem: " << muu << " "  << iupper <<  endl;


			}
		}
	}
}

uniform_distribution remaining(uniform_distribution v, const field& f, string celltok)
{
	field_factory* ff = field_factory::get_instance();

	unordered_map<string,double> intersections = ff->cell_intersection(f);
	vector<string> cells = ff->celltokens(f);
	unordered_map<string,uniform_distribution> cellmu = ff->query_mu(cells);

	for (pair<string,double> ii : intersections)
	{
		if (ii.first != celltok)
		{
			uniform_distribution this_mu (0.0,1000000.0);
			if (cellmu.count(ii.first) != 0)
				this_mu = cellmu[ii.first];
			v -= this_mu * ii.second;
		}
	}
	return v;
}

void pimprovement(const field& f, string celltok)
{

	field_factory* ff = field_factory::get_instance();

	unordered_map<string,double> intersections = ff->cell_intersection(f);
	vector<string> cells = ff->celltokens(f);
	unordered_map<string,uniform_distribution> cellmu = ff->query_mu(cells);

	uniform_distribution totalmu(0.0,0.0);
	for (pair<string,double> ii : intersections)
	{
		uniform_distribution this_mu (0.0,1000000.0);
		if (cellmu.count(ii.first) != 0)
			this_mu = cellmu[ii.first];
		totalmu += this_mu * ii.second;
	}
	
	// cerr << "this field mu est: " << totalmu << endl;

	double totalmax = round(totalmu.get_upper());
	double totalmin = round(totalmu.get_lower());

    if (totalmin==0)
        totalmin=1;

    if (totalmax==0)
        totalmax=1;

    double worst = 0.0;
    for (int tmu = totalmin; tmu <= totalmax; tmu++)
    {
        uniform_distribution mu(tmu-0.5,tmu+0.5);
        if (tmu==1)
            mu = uniform_distribution(0.0,1.5);
        
        uniform_distribution remain = remaining(mu, f, celltok) / intersections[celltok];
        uniform_distribution intremain = remain.intersection(cellmu[celltok]);

        if (intremain.range() > worst)
            worst = intremain.range();

        cerr << "mu: " << tmu << " rem: " << remain << " range: " << intremain << " " << intremain.range() << " imp: " << cellmu[celltok].range() / intremain.range() <<   endl;

    }

}

void re_calc_score(const field& f, string celltok)
{
	field_factory* ff = field_factory::get_instance();

	unordered_map<string,double> intersections = ff->cell_intersection(f);
	vector<string> cells = ff->celltokens(f);
	unordered_map<string,uniform_distribution> cellmu = ff->query_mu(cells);

	uniform_distribution totalmu(0.0,0.0);
	for (pair<string,double> ii : intersections)
	{
		uniform_distribution this_mu (0.0,1000000.0);
		if (cellmu.count(ii.first) != 0)
			this_mu = cellmu[ii.first];
		totalmu += this_mu * ii.second;
	}
	
	cerr << "this field mu est: " << totalmu << endl;

	double totalmax = round(totalmu.get_upper());
	double totalmin = round(totalmu.get_lower());

	if (totalmin <= 1.0)
		totalmin = -0.5;
	if (totalmax <= 1.0)
		totalmax = 1.0;

	uniform_distribution upper (totalmin+0.5,totalmax+0.5);

	// calculate remaining

	uniform_distribution rem_upper = remaining(upper, f, celltok) / intersections[celltok];

	// remaining < c_max

	// special ingress case any value less than 1.5 is rounded to (int)1

	if (totalmin <= 1.0)
		totalmin = 0.5;
	if (totalmax <= 1.0)
		totalmax = 2.0;

	uniform_distribution lower(totalmin-0.5,totalmax-0.5);
	uniform_distribution rem_lower = remaining(lower,f,celltok) / intersections[celltok];

	// I don't know what to expect now.
	cerr << celltok << " lower test: " << lower << " upper test: " << upper << endl; 
	cerr << "cell: " << cellmu[celltok] << " lower remain: " << rem_lower << " upper remain: " << rem_upper << endl;

    uniform_distribution ilower = cellmu[celltok].intersection(rem_lower);
    uniform_distribution iupper = cellmu[celltok].intersection(rem_upper);

	// I think the upper bounds of the lower remaining and the lower bounds of the upper remain must lie within the existing cell boundary.

    cerr << "lower intersection: " << ilower << " upper intersection: " << iupper << endl;

	//double lower_imp = cellmu[celltok].get_upper() - rem_lower.get_upper();
	//double upper_imp = rem_upper.get_lower() - cellmu[celltok].get_lower();

    uniform_distribution lower_imp = cellmu[celltok] - ilower;
    uniform_distribution upper_imp = uniform_distribution(0.0,0.0) - (cellmu[celltok] - iupper);

	cerr << "lower improvement: " << lower_imp.mean() << " : upper improvement: " << upper_imp.mean() << " : total improvement: " << lower_imp.mean() + upper_imp.mean() << endl;

	// both should be positive. larger the better...


}

int main (int argc, char* argv[])
{
    arguments ag(argc,argv);

    ag.add_req("i","improvements",false); // show improvements

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
    draw_tools otp;

    vector<field> fields = dtp.get_fields();

    field_factory* ff = field_factory::get_instance();

    uniform_distribution total(0,0);
    cout << endl;
    for (field f : fields)
    {
        unordered_map<string,double> intersections = ff->cell_intersection(f);
    	vector<string> cells = ff->celltokens(f);
    	unordered_map<string,uniform_distribution> cellmu = ff->query_mu(cells);

        uniform_distribution ftotal(0,0);

        otp.erase();
        otp.add(f);

        cout << f.geo_area() << " " << otp.to_string() << endl;

        for (string ctok : cells)
        {
            uniform_distribution tud = cellmu[ctok] * intersections[ctok];
            ftotal = ftotal + tud;
            cout << ctok << " " << cellmu[ctok] << " x " << intersections[ctok] << " = " << tud << endl;
            if (ag.has_option("i"))
                pimprovement(f,ctok);

            //predict(ctok);
        }
        cout << ftotal << " " << ftotal.range() << endl;
        total = total + ftotal;

    }
        cout << total << endl;

}