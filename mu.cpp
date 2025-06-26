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

uniform_distribution other_contribution(string celltok, const unordered_map<string,double>& intersections,const unordered_map<string,uniform_distribution>& cellmu)
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

pair<int,int> range(double area, uniform_distribution mucell, uniform_distribution othermu) 
{

	uniform_distribution totalmu1 = othermu + mucell * area;
	
	double totalmax1 = round(totalmu1.get_upper());
	double totalmin1 = round(totalmu1.get_lower());

    if (totalmin1==0)
        totalmin1=1;

    if (totalmax1==0)
        totalmax1=1;

	pair<int,int> res(totalmin1,totalmax1);

	return res;
}

vector<uniform_distribution> ranges(const field& f1, uniform_distribution mucell, string cell_token) 
{
	field_factory* ff = field_factory::get_instance();

	unordered_map<string,double> intersections1 = ff->cell_intersection(f1);
	vector<string> cells1 = ff->celltokens(f1);
	unordered_map<string,uniform_distribution> cellmu1 = ff->query_mu(cells1);

	uniform_distribution othermu1 = other_contribution(cell_token,intersections1,cellmu1);
	pair<int,int> murange = range(intersections1[cell_token], mucell,othermu1);

	vector<uniform_distribution> res;
	for (int tmu1 = murange.first; tmu1 <= murange.second; tmu1++)
    {
        uniform_distribution mu1(tmu1-0.5,tmu1+0.5);
        if (tmu1==1)
            mu1 = uniform_distribution(0.0,1.5);
        
        uniform_distribution remain1 = (mu1 - othermu1) / intersections1[cell_token]; //remaining(mu, f, celltok) / intersections[celltok];
        uniform_distribution intremain1 = remain1.intersection(mucell);
		res.push_back(intremain1);
	}
	return res;
}

uniform_distribution lowest(uniform_distribution o, vector<vector<uniform_distribution>> r, uniform_distribution c, size_t index, string ss)
{

	if (index >= r.size())
		return c;


	uniform_distribution worst (0.0,0.0);
	for (uniform_distribution u : r[index])
	{

		uniform_distribution t = o.intersection(u);
		if (t.range() == 0)
			t = o;

		stringstream iss;

		iss << ss;
		iss << "f" << index+1 << ": ";
		iss << o.range()/t.range() << " "; 

		t = c.intersection(u);
		if (t.range() == 0)
			t = c;

		uniform_distribution next_rd;
		if (index +1 < r.size())
			next_rd = lowest(o,r,t,index+1,iss.str());
		else
			next_rd = u;

		uniform_distribution id = c.intersection(next_rd);
		if (id.range() == 0)
			id = c;
		if (id.range() > worst.range())
			worst = id;

		if (index+1 == r.size())
		{
			iss << "(" << o.range() / id.range() << ")";
			cerr << iss.str() << endl;
		}

	}
	return worst;
}

vector<vector<uniform_distribution>> multi_ranges(const vector<field>& vf, uniform_distribution current_mu, string cell_token) 
{
	vector<vector<uniform_distribution>> existing;
	if (vf.size() > 0)
	{
		for (int j =0; j < vf.size(); j++)
		{
			field f = vf[j];
			vector<uniform_distribution> fd = ranges(f,current_mu,cell_token);
			existing.push_back(fd);
		}
	}
	return existing;
}

void pimprovement(const field& f, string celltok)
{

	field_factory* ff = field_factory::get_instance();

	unordered_map<string,double> intersections = ff->cell_intersection(f);
	vector<string> cells = ff->celltokens(f);
	unordered_map<string,uniform_distribution> cellmu = ff->query_mu(cells);

	uniform_distribution othermu = other_contribution(celltok,intersections,cellmu);

	pair<int,int> murange = range(intersections[celltok], cellmu[celltok], othermu);

    double worst = 0.0;
    for (int tmu = murange.first; tmu <= murange.second; tmu++)
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

void show_matrix_improvements(const vector<field>& f, string celltok)
{
	field_factory* ff = field_factory::get_instance();
	vector<string> cells;
	cells.push_back(celltok);
	unordered_map<string,uniform_distribution> cellmu = ff->query_mu(cells);

	vector<vector<uniform_distribution>> field_ranges = multi_ranges(f, cellmu[celltok], celltok);

	string ss;
	lowest(cellmu[celltok],field_ranges,cellmu[celltok],0,ss);
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
	if (ag.has_option("i") && fields.size() > 1) {
		vector<string>cc = intcells(fields);
		for (string ctok : cc)
		{
			cerr << "[" << ctok << "]" <<endl;
			show_matrix_improvements (fields,ctok);
			/*
			for (int i=0; i<fields.size(); i++)
			{
				for (int j=i+1; j<fields.size(); j++)
				{
					// compare_improvement(fields[i],fields[j],ctok);
				}
			}
			*/
		}
	}
    cout << total << endl;

}