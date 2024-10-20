#include "field_factory.hpp"

using namespace std;

namespace silicontrip {
field_factory* field_factory::ptr = 0;

field_factory::field_factory() 
{
    ifstream file_properties("portal_factory_properties.json");
    Json::Value properties;

    file_properties >> properties;

    cell_api = properties["cellurl"].asString();

}
field_factory* field_factory::get_instance()
{
    if (!ptr)
    ptr = new field_factory();
    return ptr;
}

// Hmm, identical to the portal_factory versions.
Json::Value* field_factory::read_json_from_file(const string url) const
{
    int pathsep = url.find_first_of('/');
    string path = url.substr(pathsep);

    Json::Value* result = new Json::Value;

    ifstream file(path);

    file >> *result;

    return result;
}

Json::Value* field_factory::read_json_from_http(const string url) const
{

    Json::Value* result = new Json::Value();

    stringstream str_res;
    str_res << curlpp::options::Url(url);
    str_res >> *result;

    return result;
}

vector<field>* field_factory::over_target(vector<field>*f, std::vector<point>*t) const
{
    vector<field>* fa = new vector<field>();

    for (field fi: *f)
        if (fi.inside(t))
            fa->push_back(fi);

    return fa;

}

bool geo_comparison(const field& a, const field& b)
{
    return a.geo_area() > b.geo_area();
}

vector<field>* field_factory::percentile(vector<field>*f, double percent) const
{

    vector<field>* v = new vector<field>();

    for (field fi: *f)
    {
        v->push_back(fi);
    }

    sort(v->begin(),v->end(),geo_comparison);

    int end = round((f->size() * percent / 100.0));

    v->resize(end);

    return v;

}

vector<field>* field_factory::filter_fields(vector<field>*f, vector<link>*l, team_count tc) const
{
    vector<field>* fa = new vector<field>();

    for (field fi: *f) {
        team_count bb;
        for (link li: *l) {
            if (fi.intersects(li)) {
                bb.inc_team_enum(li.get_team_enum());
            }
            if (bb > tc)
                break;
        }
        if (!(bb > tc))
            fa->push_back(fi);
                        
                
    }
    return fa;
}

vector<field>* field_factory::filter_existing_fields(vector<field>*f, vector<link>*l) const
{
    vector<field>* ff = new vector<field>();

    // Iterate through each field in the fieldArray
    for (field fi : *f) {
        int matching = 0;

    // Check if the field contains any of the lines in lineArray
        for (link li : *l) {
            if (fi.has_line(li)) {
                matching++;
            }
        }

    // If no lines are found in the field, add it to the filteredFields
        if (matching<3) {
            ff->push_back(fi);
        }
    }

    return ff;

}

bool field_factory::link_exists(vector<line>*l, int j, point p1, point p2) const
{
    for (int k=j+1; k<l->size(); k++)
    {
        line l3 = l->at(k);
        if (
                (p1 == l3.get_o_point() && p2 == l3.get_d_point()) ||
                (p1 == l3.get_d_point() && p2 == l3.get_o_point())
        ) { 
                return true;
        }
    }
    return false;

}

vector<field>* field_factory::make_fields_from_single_links(vector<line>*l) const
{
    vector<field>* fa = new vector<field>();
    for (int i =0; i<l->size(); i++)
    {
        line l1 = l->at(i);
        for (int j=i+1; j<l->size(); j++)
        {
            line l2 = l->at(j);
            
            // point l1.o == point l2.o
            if (l1.get_o_point() == l2.get_o_point()) {
                if (link_exists(l,j,l1.get_d_point(),l2.get_d_point()))
                    fa->push_back(field(l1.get_o_point(),l1.get_d_point(),l2.get_d_point()));
            } else if (l1.o_s2latlng() == l2.d_s2latlng()) {
                if (link_exists(l,j,l1.get_d_point(),l2.get_o_point()))
                    fa->push_back(field(l1.get_o_point(),l1.get_d_point(),l2.get_o_point()));
            } else if (l1.get_d_point() == l2.get_o_point()) {
                if (link_exists(l,j,l1.get_o_point(),l2.get_d_point()))
                    fa->push_back(field(l1.get_o_point(),l1.get_d_point(),l2.get_d_point()));
            } else if (l1.get_d_point() == l2.get_d_point()) {
                if (link_exists(l,j,l1.get_o_point(),l2.get_o_point()))
                    fa->push_back(field(l1.get_o_point(),l1.get_d_point(),l2.get_o_point()));
            }
        }
    }
    return fa;

}

// the argument order is important.
// two from lines1 and 1 from lines2
vector<field>* field_factory::make_fields_from_double_links(vector<line>*lk1,vector<line>*lk2) const
{
    vector<field>* fa = new vector<field>();

    for (int i =0; i<lk1->size(); i++)
    {
        line l1 = lk1->at(i);
        for (int j=i+1; j<lk1->size(); j++)
        {
            line l2 = lk1->at(j);
            
            // point l1.o == point l2.o
            if (l1.get_o_point() == l2.get_o_point()) {
                if (link_exists(lk2,-1,l1.get_d_point(),l2.get_d_point()))
                    fa->push_back(field(l1.get_o_point(),l1.get_d_point(),l2.get_d_point()));
            } else if (l1.get_o_point() == l2.get_d_point()) {
                if (link_exists(lk2,-1,l1.get_d_point(),l2.get_o_point()))
                    fa->push_back(field(l1.get_o_point(),l1.get_d_point(),l2.get_o_point()));
            } else if (l1.get_d_point() == l2.get_o_point()) {
                if (link_exists(lk2,-1,l1.get_o_point(),l2.get_d_point()))
                    fa->push_back(field(l1.get_o_point(),l1.get_d_point(),l2.get_d_point()));
            } else if (l1.get_d_point() == l2.get_d_point()) {
                if (link_exists(lk2,-1,l1.get_o_point(),l2.get_o_point()))
                    fa->push_back(field(l1.get_o_point(),l1.get_d_point(),l2.get_o_point()));
            }
        }
    }   
    return fa;
}

std::vector<field>* field_factory::make_fields_from_triple_links(vector<line>*lk1,vector<line>*lk2,vector<line>*lk3) const
{
    vector<field>* fa = new vector<field>();
    for (int i =0; i<lk1->size(); i++)
    {
        line l1 = lk1->at(i);
        for (int j=0; j<lk2->size(); j++)
        {
            line l2 = lk2->at(j);
            
            // point l1.o == point l2.o
            if (l1.get_o_point() == l2.get_o_point()) {
                if (link_exists(lk3,-1,l1.get_d_point(),l2.get_d_point()))
                    fa->push_back(field(l1.get_o_point(),l1.get_d_point(),l2.get_d_point()));
            } else if (l1.get_o_point() == l2.get_d_point()) {
                if (link_exists(lk3,-1,l1.get_d_point(),l2.get_o_point()))
                    fa->push_back(field(l1.get_o_point(),l1.get_d_point(),l2.get_o_point()));
            } else if (l1.get_d_point() == l2.get_o_point()) {
                if (link_exists(lk3,-1,l1.get_o_point(),l2.get_d_point()))
                    fa->push_back(field(l1.get_o_point(),l1.get_d_point(),l2.get_d_point()));
            } else if (l1.get_d_point() == l2.get_d_point()) {
                if (link_exists(lk3,-1,l1.get_o_point(),l2.get_o_point()))
                    fa->push_back(field(l1.get_o_point(),l1.get_d_point(),l2.get_o_point()));
            }
        }
    }
    return fa;

}

}
