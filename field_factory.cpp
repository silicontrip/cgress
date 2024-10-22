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

S2Polygon field_factory::s2polygon(const field& f) const
{ 
    vector<S2Point> loop_points;
    loop_points.push_back(f.point_at(0).s2latlng().ToPoint());
    loop_points.push_back(f.point_at(1).s2latlng().ToPoint());
    loop_points.push_back(f.point_at(2).s2latlng().ToPoint());

    S2Loop* field_loop = new S2Loop(loop_points);
    field_loop->Normalize();

    return S2Polygon(unique_ptr<S2Loop>(field_loop));
}

S2CellUnion field_factory::cells(const S2Polygon& p) const
{
    S2RegionCoverer::Options opt;
    opt.set_max_level(13);
    opt.set_min_level(0);
    opt.set_max_cells(20);
    S2RegionCoverer rc(opt);

    return rc.GetCovering(p);
}

std::unordered_map<S2CellId,double>* field_factory::cell_intersection(const S2Polygon& p) const 
{
    unordered_map<S2CellId,double>* area = new unordered_map<S2CellId,double>();
    S2CellUnion cell_union = cells(p);

    for (S2CellId cellid: cell_union)
    {
        S2Cell cell = S2Cell(cellid);
        vector<S2Point> cell_points;
        cell_points.push_back(cell.GetVertex(0));
        cell_points.push_back(cell.GetVertex(1));
        cell_points.push_back(cell.GetVertex(2));
        cell_points.push_back(cell.GetVertex(3));
        S2Loop* cell_loop = new S2Loop(cell_points);
        cell_loop->Normalize();
        S2Polygon cell_poly = S2Polygon(unique_ptr<S2Loop>(cell_loop));

        S2Polygon int_poly;
        int_poly.InitToIntersection(p, cell_poly);

        //polyArea.put(cellid,new Double(intPoly.getArea() *  earthRadius * earthRadius));
        double sqkm = S2Earth::SteradiansToSquareKm(int_poly.GetArea());
        //uint64 id = cellid.id();
        pair<S2CellId,double> area_pair (cellid,sqkm);
        area->insert(area_pair);

    }

    return area;
}

Json::Value* field_factory::json_from_array(const vector<string>& desc) const
{
    Json::Value* res;
    if (cell_api.substr(0,4) == "file")
    {
        res = read_json_from_file(cell_api);
    } else {
        Json::Value query;

        int count=0;
        for (string title: desc)
        {
            query[count++] = title;
        }
        ostringstream jplist;
        jplist << query;
        string url = cell_api+"?mu="+curlpp::escape(jplist.str());
        res = read_json_from_http(url);
    }

    return res;
}

unordered_map<string, uniform_distribution>field_factory::query_mu_from_servlet(const vector<string>& cell_tokens) const
{
    unordered_map<string, uniform_distribution> result;
    Json::Value* rv = json_from_array(cell_tokens);
    Json::Value def = 0;
    for (string key: rv->getMemberNames())
    {
        double lower = rv->get(key,def)[0].asDouble();
        double upper = rv->get(key,def)[1].asDouble();
        uniform_distribution ud = uniform_distribution(lower,upper);
        result[key]=ud;
    }

    return result;
}
unordered_map<string, uniform_distribution>field_factory::query_mu(const vector<string>& cell_tokens)
{
    unordered_map<std::string, uniform_distribution> result; // = new unordered_map<std::string, uniform_distribution>();

        // List to collect tokens that need to be queried from servlet
        vector<string> tokens_to_query;

        // Step 1: Retrieve cached MU values
        for (string token : cell_tokens) {
            if (mu_cache.count(token)) {
                // If value exists in cache, retrieve it
                result[token] = mu_cache.at(token);
            } else {
                // If not in cache, add to tokensToQuery list
                tokens_to_query.push_back(token);
            }
        }
        if (tokens_to_query.size()) {
            unordered_map<string, uniform_distribution> servlet_mu_values = query_mu_from_servlet(tokens_to_query);

            // Step 3: Cache retrieved MU values
            for (pair<string, uniform_distribution> entry : servlet_mu_values) {
                string token = entry.first;;
                uniform_distribution mu_value = entry.second;

                // Cache the retrieved value
                mu_cache[token]= mu_value;

                // Add to the result map
                result[token]= mu_value;
            }
        }

        return result;
    }


int field_factory::calculate_mu(const S2Polygon& p)
{
    unordered_map<S2CellId, double>* cell_intersections = cell_intersection(p);
    double total_mu = 0.0;

        // Create a map to store the mu/km2 values for each S2CellId
    unordered_map<S2CellId, uniform_distribution> mu_map;

        // Prepare a list of S2CellIds to query
    vector<string> cell_ids_to_query;
    for (pair<S2CellId,double>cell_pair : *cell_intersections) {
        cell_ids_to_query.push_back(cell_pair.first.ToToken());
    }

        // Query MU/km2 values from cache
    unordered_map<string, uniform_distribution> mu_values = query_mu(cell_ids_to_query);

        // Populate muMap with retrieved mu/km2 values
    for (pair<string, uniform_distribution> entry : mu_values) {
        S2CellId cellId = S2CellId::FromToken(entry.first);
        mu_map[cellId] = entry.second;
    }

        // Calculate total MU
    for (pair<S2CellId, double> entry : *cell_intersections) {
        S2CellId cell_id = entry.first;
            double intersection_area = entry.second;
            if (mu_map.count(cell_id)) {
                uniform_distribution muPerKm2 = mu_map.at(cell_id);
                double average_mu = muPerKm2.mean();
                total_mu += intersection_area * average_mu;
            }
        }

        return (int) round(total_mu);

}

int field_factory::get_est_mu(const field& f) 
{
    S2Polygon p = s2polygon(f);
    return calculate_mu(p);
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
                    fa->emplace_back(field(l1.get_o_point(),l1.get_d_point(),l2.get_d_point()));
            } else if (l1.o_s2latlng() == l2.d_s2latlng()) {
                if (link_exists(l,j,l1.get_d_point(),l2.get_o_point()))
                    fa->emplace_back(field(l1.get_o_point(),l1.get_d_point(),l2.get_o_point()));
            } else if (l1.get_d_point() == l2.get_o_point()) {
                if (link_exists(l,j,l1.get_o_point(),l2.get_d_point()))
                    fa->emplace_back(field(l1.get_o_point(),l1.get_d_point(),l2.get_d_point()));
            } else if (l1.get_d_point() == l2.get_d_point()) {
                if (link_exists(l,j,l1.get_o_point(),l2.get_o_point()))
                    fa->emplace_back(field(l1.get_o_point(),l1.get_d_point(),l2.get_o_point()));
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
