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

uniform_distribution other_contribution(const field& f, string celltok, const unordered_map<string,double>& intersections,const unordered_map<string,uniform_distribution>& cellmu)
{

	uniform_distribution othermu(0.0,0.0);
	for (pair<string,double> ii : intersections)
	{
		if (ii.first != celltok)
		{
			uniform_distribution this_mu (0.0,1000000.0);
			if (cellmu.count(ii.first) != 0)
				this_mu = cellmu.at(ii.first);
			othermu += this_mu * ii.second;
		}
	}
	return othermu;
}

void compare_improvement(const field& f1, const field& f2, string cell_token)
{

	field_factory* ff = field_factory::get_instance();

	unordered_map<string,double> intersections1 = ff->cell_intersection(f1);
	vector<string> cells1 = ff->celltokens(f1);
	unordered_map<string,uniform_distribution> cellmu1 = ff->query_mu(cells1);

	uniform_distribution othermu1 = other_contribution(f1,cell_token,intersections1,cellmu1);

	uniform_distribution totalmu1 = othermu1 + cellmu1[cell_token] * intersections1[cell_token];

	unordered_map<string,double> intersections2 = ff->cell_intersection(f2);
	vector<string> cells2 = ff->celltokens(f2);
	unordered_map<string,uniform_distribution> cellmu2 = ff->query_mu(cells2);

	uniform_distribution othermu2 = other_contribution(f2,cell_token,intersections2,cellmu2);

	// cerr << "this field mu est: " << totalmu << endl;

	double totalmax1 = round(totalmu1.get_upper());
	double totalmin1 = round(totalmu1.get_lower());

    if (totalmin1==0)
        totalmin1=1;

    if (totalmax1==0)
        totalmax1=1;

	//if (totalmin1 == totalmax1)
	//	return 1.0;

	double current = cellmu1[cell_token].range();
    double worst = DBL_MAX;
	double worst1 = 0.0;
    for (int tmu1 = totalmin1; tmu1 <= totalmax1; tmu1++)
    {
        uniform_distribution mu1(tmu1-0.5,tmu1+0.5);
        if (tmu1==1)
            mu1 = uniform_distribution(0.0,1.5);
        
        uniform_distribution remain1 = (mu1 - othermu1) / intersections1[cell_token]; //remaining(mu, f, celltok) / intersections[celltok];
        uniform_distribution intremain1 = remain1.intersection(cellmu1[cell_token]);


		uniform_distribution totalmu2 = othermu2 + intremain1 * intersections2[cell_token];

		double totalmax2 = round(totalmu2.get_upper());
		double totalmin2 = round(totalmu2.get_lower());

    	if (totalmin2==0)
        	totalmin2=1;

    	if (totalmax2==0)
        	totalmax2=1;

		if (intremain1.range() > worst1)
			worst1 = intremain1.range();

		double imp1 = cellmu1[cell_token].range() / intremain1.range();
		cerr << "f1: [" << tmu1 << ":" << imp1 << "] f2:";

		bool first = true;
		double worst2 = 0.0;
		for (int tmu2 = totalmin2; tmu2 <= totalmax2; tmu2++)
    	{
        	uniform_distribution mu2(tmu2-0.5,tmu2+0.5);
        	if (tmu2==1)
            	mu2 = uniform_distribution(0.0,1.5);
        
        	uniform_distribution remain2 = (mu2 - othermu2) / intersections2[cell_token]; //remaining(mu, f, celltok) / intersections[celltok];
        	uniform_distribution intremain2 = remain2.intersection(intremain1);

			// intremain2 field2 improvement combined field1

			if (intremain2.range() > worst2)
				worst2 = intremain2.range();

			double imp2 = intremain1.range() / intremain2.range();

			cerr << " [" << tmu2 << ":" << imp2 << "]";

		}

		double cimp = imp1 * intremain1.range() / worst2;
		
		cerr << " (" << cimp << ")" << endl;

		if (cimp < worst)
			worst = cimp;

    }

	//cerr << cellmu1[cell_token].range() / worst1 << " == " << worst << endl;
	
	//return (abs (cellmu1[cell_token].range() / worst1 - worst) > epsilon);

	//return cellmu[celltok].range() / worst;

}

uniform_distribution uniform_improvement(const field& f, uniform_distribution mucell, string cell_token)
{

	field_factory* ff = field_factory::get_instance();

	unordered_map<string,double> intersections = ff->cell_intersection(f);
	vector<string> cells = ff->celltokens(f);
	unordered_map<string,uniform_distribution> cellmu = ff->query_mu(cells);

	uniform_distribution othermu = other_contribution(f,cell_token,intersections,cellmu);

	uniform_distribution totalmu = othermu + mucell * intersections[cell_token];
	
	// cerr << "this field mu est: " << totalmu << endl;

	double totalmax = round(totalmu.get_upper());
	double totalmin = round(totalmu.get_lower());

    if (totalmin==0)
        totalmin=1;

    if (totalmax==0)
        totalmax=1;

	if (totalmin == totalmax)
		return mucell;

	double current = mucell.range();
    double worst = 0.0;
	uniform_distribution worst_dist;
    for (int tmu = totalmin; tmu <= totalmax; tmu++)
    {
        uniform_distribution mu(tmu-0.5,tmu+0.5);
        if (tmu==1)
            mu = uniform_distribution(0.0,1.5);
        
        uniform_distribution remain = (mu - othermu) / intersections[cell_token]; //remaining(mu, f, celltok) / intersections[celltok];
        uniform_distribution intremain = remain.intersection(mucell);

		double intremainrange = intremain.range();
		
		if (intremainrange >= current) // might need to epsilon this
			return remain;
        if (intremainrange > worst)
		{
            worst = intremain.range();
			worst_dist = intremain;
		}
        cerr << "mu: " << tmu << " rem: " << remain << " range: " << intremain.range() << " imp: " << cellmu[cell_token].range() / intremain.range() <<   endl;

    }

	return worst_dist;
}

double multi_improvement(const vector<field>& vf, string cell_token)
{
	double imp = 1.0;
	if (vf.size() > 0)
	{
		field_factory* ff = field_factory::get_instance();

		vector<string> cells = ff->celltokens(vf[0]);
		unordered_map<string,uniform_distribution> cellmu = ff->query_mu(cells);

		uniform_distribution origmu = cellmu[cell_token];
		uniform_distribution current = cellmu[cell_token];

		for (int i=vf.size()-1; i>=0 ; i--)
		{
			const field& f = vf[i];
			uniform_distribution uimp = uniform_improvement(f,current,cell_token);
			cerr << imp << " * " << current.range() / uimp.range() << " " << current << " * " << uimp  << " = ";
			imp = imp * current.range() / uimp.range();
			cerr << imp << endl;
			current = uimp;
		}
	
	//uniform_distribution cimp = uniform_improvement(fi,current);
	//cerr << imp << " * " << current.range() / cimp.range() << " " << current << " * " << cimp << endl;

		cerr << "total: " << origmu.range() / current.range() << endl;
	}
	//imp = imp * current.range() / cimp.range();

	return imp;
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

vector<string> intcells (vector<field> f)
{
    field_factory* ff = field_factory::get_instance();
	unordered_set<string> mcells;
	for (field fi : f)
	{
		vector<string> ocells = ff->celltokens(fi);
		for (string oc : ocells)
		{
			bool ofound = true;
			for (field fj : f)
			{
				vector<string> icells = ff->celltokens(fj);
				bool ifound = false;
				for (string ic : icells)
				{
					if (ic == oc)
					{
						ifound = true;
						break;
					}	
				}
				if (!ifound)
				{
					ofound = false;
					break;
				}
			}
			if (ofound)
				mcells.insert(oc);
		}
	}
	vector<string> vcells(mcells.begin(),mcells.end());
	return vcells;
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
            if (ag.has_option("i")) {
				// multi_improvement(fields,ctok);
                pimprovement(f,ctok);
			}
            //predict(ctok);
        }
        cout << ftotal << " " << ftotal.range() << endl;
        total = total + ftotal;

    }
	if (ag.has_option("i")) {
		vector<string>cc = intcells(fields);
		for (string ctok : cc)
		{
			cerr << "[" << ctok << "]" <<endl;
			for (int i=0; i<fields.size(); i++)
			{
				for (int j=i+1; j<fields.size(); j++)
				{
					compare_improvement(fields[i],fields[j],ctok);
				}
			}
		}
	}
    cout << total << endl;

}