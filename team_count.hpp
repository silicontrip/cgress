#ifndef SILICONTRIP_TEAM_COUNT_HPP
#define SILICONTRIP_TEAM_COUNT_HPP

#include <string>

#include "link.hpp"

namespace silicontrip {
class team_count {
    private:
        int resistance = -1;
        int enlightened = -1;
        int neutral = -1;
        bool exists = false;

    public:

        team_count();
        team_count(int enl, int res, int neu);
        team_count(std::string enl, std::string res, std::string neu);
        //team_count(int argc, char *argv[]);

        void inc_team(std::string team);
        void inc_team_enum(enum ingressteam team);

        void inc_enlightened();
        void inc_resistance();
        void inc_neutral();

        int get_resistance() const;
        int get_enlightened() const;
        int get_neutral() const;

        void set_resistance(int i);
        void set_enlightened(int i);
        void set_neutral(int i);

        void add_resistance(int i);
        void add_enlightened(int i);
        void add_neutral(int i);

        bool no_resistance() const;
        bool no_enlightened() const;
        bool no_neutral() const;

        // the logic for these operators is inconsistent. However makes sense for our use case.
        // Any are greater than.  All must be less than.
        bool operator>(const team_count& tc) const;
        bool operator<=(const team_count& tc) const;

        bool any_resistance_blockers() const;
        bool any_enlightened_blockers() const;
        bool any_neutral_blockers() const;
        bool any_blockers() const;
        bool dont_care() const;

        int max() const;
        int min() const;


        std::string to_string() const;
};
}
std::ostream& operator<<(std::ostream& os, const silicontrip::team_count& t);

#endif
