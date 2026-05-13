
#include <cmath>
#include <cstdint>
#include <iostream>
#include <chrono>
#include <thread>
#include <random>
#include <unordered_set>
#include <unordered_map>

#include "run_timer.hpp"
#include "arguments.hpp"
#include "team_count.hpp"
#include "portal_factory.hpp"
#include "link_factory.hpp"
#include "field_factory.hpp"
#include "draw_tools.hpp"
#include "link.hpp"
#include "portal.hpp"

using namespace std;
using namespace silicontrip;

class orion {

    private:
	draw_tools dt;
	run_timer rt;
    vector<portal>portals;
    unordered_set<line> link_filter;
    int rethrows;

    vector<portal> reachable (vector<line>l,bool reverse);
    double num_reachable (vector<line>l,bool reverse);
    double get_score (portal o, vector<portal>s,vector<double>sa,uint8_t* sol, bool reverse);
    double simulate(double best_score,portal o, vector<portal>s,vector<double>sa,uint8_t* sol);
    bool is_active(int i, uint8_t* sol);
    void flip(int i, uint8_t* sol);
    vector<line>get_spine(portal o, vector<portal>s,vector<double>sa,uint8_t* sol);
    vector<line>get_rspine(portal o, vector<portal>s,vector<double>sa,uint8_t* sol);

    double get_gap(portal o, vector<portal>s,vector<double>sa,uint8_t* sol);
    void print_plan(portal o, vector<portal>s,vector<double>sa,uint8_t* sol, bool reverse);

    public:

    orion(draw_tools dts, run_timer rtm,vector<portal>&p, int r, unordered_set<line> filter);
    double anneal(double best_score,portal o, vector<portal>s,vector<double>sa);

};

orion::orion(draw_tools dts, run_timer rtm, vector<portal>&p, int r, unordered_set<line> filter)
{
    dt = dts;
    rt = rtm;
    portals = p;
    rethrows = r;
    link_filter = filter;
}

double orion::num_reachable (vector<line>l,bool reverse)
{
    if (l.empty()) return 0;
    point o = l.back().get_o_point();

    vector<portal>pts;
    pts.assign(portals.begin(),portals.end());
    int num = 0;
    for (line li : l)
    {
        vector<portal>npts;
        for (portal pp : pts)
        {
            if (link_filter.find(line(o,pp)) == link_filter.end()) continue;
            double ps = s2pred::Sign(li.o_s2latlng().ToPoint(),li.d_s2latlng().ToPoint(),pp.s2latlng().ToPoint());
                        
            if (reverse)
            {
                if (ps < 0)
                    npts.push_back(pp);
            } else {
                if (ps > 0)
                    npts.push_back(pp);
            }
        }
        //num += npts.size();
        if (npts.size() == 0)
            num--;
        pts = npts;
    }
    if (num<0)
        return num;
    return pts.size();
}

vector<portal> orion::reachable (vector<line>l, bool reverse)
{
    if (l.empty()) return vector<portal>();
    point o = l.back().get_o_point();

    vector<portal>pts;
    pts.assign(portals.begin(),portals.end());

    for (line li : l)
    {
        vector<portal>npts;
        for (portal pp : pts)
        {
            if (link_filter.find(line(o,pp)) == link_filter.end()) continue;
            double ps = s2pred::Sign(li.o_s2latlng().ToPoint(),li.d_s2latlng().ToPoint(),pp.s2latlng().ToPoint());
                        
            if (reverse)
            {
                if (ps < 0)
                    npts.push_back(pp);
            } else {
                if (ps > 0)
                    npts.push_back(pp);
            }
        }
        if (npts.size() == 0)
            return npts;
        pts = npts;
    }
    return pts;
}

bool orion::is_active(int i, uint8_t* sol)
{
    size_t byte = i / 8;
    size_t bit = 1 << (7 - (i % 8));
    return (sol[byte] & bit) != 0;
}

void orion::flip(int i, uint8_t* sol)
{
    size_t byte = i / 8;
    size_t bit = 1 << (7 - (i % 8));
    sol[byte] = sol[byte] ^ bit;
}

double orion::get_gap(portal o, vector<portal>s,vector<double>sa,uint8_t* sol)
{
    size_t spinelen = s.size();
    double gap = 0;
    double old = -1;
    double first;
    double last;
    int start=0;
      int active_count = 0;                                                                                               

    for (int i=0; i<spinelen; i++)
    {
        if (is_active(i,sol))
        {
            active_count++;
            if (old == -1)
            {
                old = sa[i];
                first = old;
            } else {
                double tgap = fabs(sa[i] - old);
                old = sa[i];
                last = old;
                if (tgap>gap)
                    gap = tgap;
                if (tgap > M_PI)
                    start = i;
            }
        }
    }
    if (active_count < 2) return 0;

    double tgap = first - (last - M_PI * 2 );
    if (tgap>gap)
        gap = tgap;

    return gap;
}

vector<line>orion::get_rspine(portal o, vector<portal>s,vector<double>sa,uint8_t* sol)
{
    size_t spinelen = s.size();
    double gap = 0;
    double old = -1;
    double first;
    double last;
    int start=-1;
    for (int i=spinelen-1; i>=0; i--)
    {
        if (is_active(i,sol))
        {
            if (start == -1)
                start = i;
            if (old == -1)
            {
                old = sa[i];
                first = old;
            } else {
                double tgap = fabs(sa[i] - old);
                old = sa[i];
                last = old;
                if (tgap>gap)
                    gap = tgap;
                if (tgap > M_PI)
                    start = i;
            }
        }
    }
    //if (debug)
    //    cerr << "start: " << start << endl;
    double tgap = first - (last - M_PI * 2 );
    if (tgap>gap)
        gap = tgap;

    vector<line> spine;

    if (gap <= M_PI)
        return spine;

    portal oldp=s[start];

    for (int i=start-1; i>=0; i--)
    {
        if (is_active(i,sol))
        {
            line l(s[i],oldp);
            if (link_filter.find(l) == link_filter.end()) return vector<line>();

            //if (debug)
            //    cerr << i << ": " << spine.size() << " ang: " << sa[i] <<  " l: " << l << endl;

            spine.push_back(l);
            oldp = s[i];
        }
    }

    for (int i=spinelen-1; i>start; i--)
    {
        if (is_active(i,sol))
        {
            line l(s[i],oldp);
            if (link_filter.find(l) == link_filter.end()) return vector<line>();

            //if (debug)
            //    cerr << i << ": " << spine.size() << " ang: " << sa[i] <<  " l: " << l << endl;
            spine.push_back(l);
            oldp = s[i];
        }
    }

    line l(o,oldp);
    if (link_filter.find(l) == link_filter.end()) return vector<line>();
    spine.push_back(l);
    return spine;
}


vector<line>orion::get_spine(portal o, vector<portal>s,vector<double>sa,uint8_t* sol)
{
    size_t spinelen = s.size();
    double gap = 0;
    double old = -1;
    double first;
    double last;
    int start=-1;
    for (int i=0; i<spinelen; i++)
    {
        if (is_active(i,sol))
        {
            if (start == -1)
                start = i;
            if (old == -1)
            {
                old = sa[i];
                first = old;
            } else {
                double tgap = fabs(sa[i] - old);
                old = sa[i];
                last = old;
                if (tgap>gap)
                    gap = tgap;
                if (tgap > M_PI)
                    start = i;
            }
        }
    }
    //if (debug)
    //    cerr << "start: " << start << endl;
    double tgap = first - (last - M_PI * 2 );
    if (tgap>gap)
        gap = tgap;

    vector<line> spine;

    if (gap <= M_PI)
        return spine;

    portal oldp=s[start];
    for (int i=start+1; i<spinelen; i++)
    {
        if (is_active(i,sol))
        {
            line l(s[i],oldp);
            if (link_filter.find(l) == link_filter.end()) return vector<line>();

            //if (debug)
            //    cerr << i << ": " << spine.size() << " ang: " << sa[i] <<  " l: " << l << endl;
            spine.push_back(l);
            oldp = s[i];
        }
    }

    for (int i=0; i<start; i++)
    {
        if (is_active(i,sol))
        {
            line l(s[i],oldp);
            if (link_filter.find(l) == link_filter.end()) return vector<line>();

            //if (debug)
            //    cerr << i << ": " << spine.size() << " ang: " << sa[i] <<  " l: " << l << endl;

            spine.push_back(l);
            oldp = s[i];
        }
    }
    line l(o,oldp);
    if (link_filter.find(l) == link_filter.end()) return vector<line>();
    spine.push_back(l);
    return spine;
}

double orion::get_score (portal o, vector<portal>s,vector<double>sa,uint8_t* sol,bool reverse)
{
    double gap = get_gap(o,s,sa,sol);
    if (gap <= M_PI)
    {
        //cerr << "gap: " << gap << endl;
        return (180.0 / M_PI * gap) - 180.0;
    }
    vector<line>spine;
    if (reverse)
        spine = get_rspine(o,s,sa,sol);
    else
        spine = get_spine(o,s,sa,sol);

    if (spine.size() < 3)
        return 0;
    double r = num_reachable(spine,reverse);
    if (r<1)
        return r;
    if (rethrows >0 && r > rethrows)
        r = rethrows;
    return spine.size() - 2 + ((spine.size() - 2) * 2 + 1) * r;
}

void orion::print_plan(portal o, vector<portal>s,vector<double>sa,uint8_t* sol, bool reverse)
{
    vector<line>spine;
    if (reverse)
        spine = get_rspine(o,s,sa,sol);
    else
        spine = get_spine(o,s,sa,sol);

    dt.erase();
    //dt.add(o);
    for (int i =0; i<spine.size(); i++)
    {
        line li = spine[i];
        dt.add(li);
        if (i < spine.size()-1)
        {
            line l(li.get_o_point(),o);
            dt.add(l);
        }
    }
    vector<portal> pp = reachable(spine,reverse);
    cout << "reachable: " << pp.size() << " spine: " << spine.size() << endl;
    for (portal p : pp)
        dt.add(p);
    cout << dt.to_string() <<endl<<endl;
}

double orion::simulate(double best_score, portal o, vector<portal>s,vector<double>sa,uint8_t* sol)
{
    double temp = 8.0;
    int iterations = 2000;
    
    size_t spinelen = s.size();
    random_device rd;
	mt19937 gen(rd());
    uniform_int_distribution<uint32_t> dist(0, spinelen-1);
	uniform_real_distribution<float> rand(0,1);

    size_t pbytes = ( spinelen + 7 ) / 8;
    uint8_t* tsol = (uint8_t*)calloc(pbytes,1);
    uint8_t* osol = tsol;
    double score = get_score(o,s,sa,sol,false);
    double tscore = get_score(o,s,sa,sol,true);

    if (tscore > score)
        score = tscore;

    while (temp > 1e-6) 
    {
        for (int it=0; it < iterations; it++)
        {
            memcpy(tsol,sol,pbytes);
            uint32_t f = dist(gen);

            flip(f,tsol);

            tscore = get_score(o,s,sa,tsol,false);
			if (tscore > score || exp((tscore - score) / temp) > rand(gen))
            {
                // could swap pointers
                score = tscore;
                //memcpy(sol,tsol,pbytes);
                uint8_t* t = sol;
                sol = tsol;
                tsol = t;
                if (score > best_score)
                {
                    cout << score << "(" << temp << "/" << it << ")" << endl;
                    if (score >= 1)
                        print_plan(o,s,sa,sol,false);
                    best_score = score;
                }
            }

             tscore = get_score(o,s,sa,tsol,true);
			if (tscore > score || exp((tscore - score) / temp) > rand(gen))
            {
                // could swap pointers
                score = tscore;
                //memcpy(sol,tsol,pbytes);
                uint8_t* t = sol;
                sol = tsol;
                tsol = t;
                if (score > best_score)
                {
                    cout << score << "(" << temp << "/" << it << ")" << endl;
                    if (score >= 1)
                        print_plan(o,s,sa,sol,true);
                    best_score = score;
                }
            }

        }
        temp *= 0.95;
    }
    free(osol);
    return best_score;
}

double orion::anneal(double best_score, portal o, vector<portal>s, vector<double>sa)
{
    random_device rd;
	mt19937 gen(rd());

    size_t spinelen = s.size();
    size_t pbytes = ( spinelen + 7 ) / 8;

    uint8_t* solution = (uint8_t*)malloc(pbytes);
    uniform_int_distribution<uint8_t> dist(0, 1);

    for (int i=0; i < spinelen; i++)
    {
        if (dist(gen))
        {
            size_t byte = i / 8;
            size_t bit = 1 << (7 - (i % 8));
            solution[byte] = solution[byte] | bit;
        }
    }

    best_score = simulate(best_score,o,s,sa,solution);
    free(solution);
    return best_score;
}

void print_usage()
{
		cerr << "Usage:" << endl;
		cerr << "orion [options] <portal cluster>" << endl;
		cerr << "Options:" << endl;
		cerr << " -E <number>       Limit number of Enlightened Blockers" << endl;
		cerr << " -R <number>       Limit number of Resistance Blockers" << endl;
		cerr << " -N <number>       Limit number of Machina Blockers" << endl;
		cerr << " -C <#colour>      Set Drawtools output colour" << endl;
		cerr << " -L                Set Drawtools to output as polylines" << endl;
		cerr << " -r <number>       Limit the number of rethrows" << endl;
		cerr << " -S <portal cluster> source portal cluster" << endl;
}

int main (int argc, char* argv[])
{
	run_timer rt;

    arguments ag(argc,argv);
    int rethrows=0;

    ag.add_req("E","enlightened",true); // max enlightened blockers
	ag.add_req("R","resistance",true); // max resistance blockers
	ag.add_req("N","machina",true); // max machina blockers
	
	ag.add_req("C","colour",true); // drawtools colour
	ag.add_req("L","polylines",false); // output as polylines
	ag.add_req("M","MU",false); // calculate as MU
	ag.add_req("r","rethrows",true); // limit rethrows (flip and go to new portal)
    ag.add_req("S","source",true); // source portal cluster
	ag.add_req("m","minimum",true); // minimum number of layers
	ag.add_req("h","help",false);

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

	team_count tc = team_count(ag.get_option_for_key("E"),ag.get_option_for_key("R"),ag.get_option_for_key("N"));
	draw_tools dt;

	if (ag.has_option("C")) {
		cerr << "set colour: " << ag.get_option_for_key("C") << endl;
		dt.set_colour(ag.get_option_for_key("C"));
	}

	if (ag.has_option("L"))
		dt.set_output_as_polyline();

    if (ag.has_option("r"))
        rethrows = ag.get_option_for_key_as_int("r");

	portal_factory* pf = portal_factory::get_instance();
	link_factory* lf = link_factory::get_instance();
	field_factory* ff = field_factory::get_instance();

	cerr << "== Reading links and portals ==" << endl;
	rt.start();

	vector<silicontrip::link> links;


    try {
        if (ag.argument_size() == 1)
	    {
            vector<portal> portals;
            
            portals = pf->cluster_from_description(ag.get_argument_at(0));

            cerr << "== " << portals.size() << " portals read. in " << rt.split() << " seconds. ==" << endl;

            cerr << "== getting links ==" << endl;
                                    
            links = lf->get_purged_links(portals);
                                    
            cerr <<  "== " << links.size() << " links read. in " << rt.split() <<  " seconds ==" << endl;

            cerr << "== generating potential links ==" << endl;

            vector<line> li = lf->make_lines_from_single_cluster(portals);
            cerr << "all links: " << li.size() << endl;
            li = lf->filter_links(li,links,tc);
                        
            cerr << "purged links: " << li.size() << endl;
            cerr << "==  links generated " << rt.split() <<  " seconds ==" << endl;

            unordered_set<line> filter(li.begin(),li.end());

            /*
            cerr << "== build link cache ==" << endl;

            unordered_map<point,unordered_set<line>> link_cache;
            for (silicontrip::line l : li)
            {
                point opoint = l.get_o_point();
                point dpoint = l.get_d_point();

                if (link_cache.count(opoint))
                {
                    link_cache[opoint].insert(l);
                } else {
                    unordered_set<line> us;
                    us.insert(l);
                    link_cache[opoint] = us;
                }
                if (link_cache.count(dpoint))
                {
                    link_cache[dpoint].insert(l);
                } else {
                    unordered_set<line> us;
                    us.insert(l);
                    link_cache[dpoint] = us;     
                }
            }
            cerr << "==  cache generated " << rt.split() <<  " seconds ==" << endl;
            */


            double best_score = -2000;
            cerr << "== search base links ==" << endl;
            orion o(dt,rt,portals,rethrows,filter);

            for (size_t i = 0; i < portals.size(); i++)
            {
                portal oportal=portals[i];
                unordered_map<double, portal> spineangle;
                for (size_t j=0; j < portals.size(); j++)
                {
                    if (i != j)
                    {
                        portal dportal = portals[j];
                        if (filter.find(line(oportal,dportal)) != filter.end())
                        {
                            double rad = oportal.bearing_to(dportal).radians();
                            spineangle[rad] = dportal;
                        }
                    }
                }
                vector<double> vs ;
                vs.reserve(spineangle.size());
                for (pair<double,portal>a : spineangle)
                {
                    vs.push_back(a.first);
                }
                std::sort(vs.begin(), vs.end());
                vector<portal>spineorder;
                for (double b : vs)
                {
                    spineorder.push_back(spineangle[b]);
                }

                best_score = o.anneal(best_score,oportal,spineorder,vs);

                /*
                // requires change to sign
                std::sort(vs.begin(), vs.end(),std::greater<double>());
                vector<portal>rspineorder;
                for (double b : vs)
                {
                    rspineorder.push_back(spineangle[b]);
                }
                best_score = o.anneal(best_score,oportal,rspineorder,vs);
                */
            }

        }

    } catch (exception &e) {
		cerr << "An Error occured: " << e.what() << endl;
        return 1;
	}
	cerr <<  "== Finished. " << rt.split() << " elapsed time. " << rt.stop() << " total time." << endl;

    return 0;

}
