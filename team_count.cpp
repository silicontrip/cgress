#include "team_count.hpp"

using namespace std;

namespace silicontrip {

    team_count::team_count() { ; }
    team_count::team_count(int enl, int res, int neu) { 
        enlightened = enl;
        resistance = res;
        neutral = neu;
    }
    team_count::team_count(string enl, string res, string neu)
    {
        char *e;
        if(enl.size()>0) { enlightened = strtol(enl.c_str(), &e, 10); }
        if(res.size()>0) { resistance = strtol(res.c_str(), &e, 10); }
        if(neu.size()>0) { neutral = strtol(neu.c_str(), &e, 10); }
    }

   void team_count::inc_team(std::string team)
   {
        if (team[0]=='E') { inc_team_enum(ENLIGHTENED);}
        if (team[0]=='R') { inc_team_enum(RESISTANCE);}
        if (team[0]=='N') { inc_team_enum(NEUTRAL);}
   }

    void team_count::inc_team_enum(enum ingressteam team)
    {
        if(team==ENLIGHTENED) { inc_enlightened(); }
        if(team==RESISTANCE) { inc_resistance(); }
        if(team==NEUTRAL) { inc_neutral(); }
    }

    void team_count::inc_enlightened()
    {
        if (enlightened == -1)
            enlightened = 1;
        else
            enlightened++;
    }

    void team_count::inc_resistance()
    {
        if (resistance == -1)
            resistance = 1;
        else
            resistance++;
    }

    void team_count::inc_neutral()
    {
        if (neutral == -1)
            neutral = 1;
        else
            neutral++;
    }

    int team_count::get_enlightened() const { return enlightened; }
    int team_count::get_neutral() const { return neutral; }
    int team_count::get_resistance() const { return resistance; }

    void team_count::set_enlightened(int i) { enlightened = i; }
    void team_count::set_resistance(int i) { resistance = i; }
    void team_count::set_neutral(int i) { neutral = i; }

    void team_count::add_enlightened(int i)
    {
        if (enlightened == -1)
            enlightened = i;
        else
            enlightened+=i;
    }

    void team_count::add_resistance(int i)
    {
        if (resistance == -1)
            resistance = i;
        else
            resistance+=i;
    }

    void team_count::add_neutral(int i)
    {
        if (neutral == -1)
            neutral = i;
        else
            neutral+=i;
    }

    bool team_count::no_resistance() const { return resistance == -1; }
    bool team_count::no_enlightened() const { return enlightened == -1; }
    bool team_count::no_neutral() const { return neutral == -1; }

    bool team_count::more_than (team_count tc) const
    {
        bool res = false;
        bool enl = false;
        bool neu = false;

        if (!tc.no_enlightened() && !no_enlightened())
            enl = get_enlightened() > tc.get_enlightened();
        if (!tc.no_resistance() && !no_resistance())
            res = get_resistance() > tc.get_resistance();
        if (!tc.no_neutral() && !no_neutral())
            neu = get_neutral() > tc.get_neutral();

        return res || enl || neu;

    }


    bool team_count::any_resistance_blockers() const { return resistance > 0; }
    bool team_count::any_enlightened_blockers() const { return enlightened > 0; }
    bool team_count::any_neutral_blockers() const { return neutral > 0; }

    bool team_count::any_blockers() const { return resistance > 0 || enlightened > 0 || neutral > 0; }
    bool team_count::dont_care() const { return resistance == -1 && enlightened == -1 && neutral == -1; }

    string team_count::to_string() const {
        return "ENL: " + std::to_string(enlightened) + " RES: "+ std::to_string(resistance)+ " NEU: " + std::to_string(neutral);
    }

}
std::ostream& operator<<(std::ostream& os, const silicontrip::team_count& t)
{
    os << t.to_string();
    return os;
}