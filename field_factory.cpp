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

/************************************************
 * FILTER Functions
*************************************************/

// return fields that cover the target points.
vector<field> field_factory::over_target(const vector<field>&f, const std::vector<point>&t) const
{
    vector<field> fa;

    for (field fi: f)
        if (fi.inside(t))
            fa.push_back(fi);

    return fa;

}

bool geo_comparison(const field& a, const field& b)
{
    return a.geo_area() > b.geo_area();
}

// return the largest percent of fields
vector<field> field_factory::percentile(const vector<field>&f, double percent) const
{

    vector<field> v;

    for (field fi: f)
    {
        v.push_back(fi);
    }

    sort(v.begin(),v.end(),geo_comparison);

    int end = round((f.size() * percent / 100.0));

    v.resize(end);

    return v;

}

// return fields that have fewer blockers than specified by team_count
vector<field> field_factory::filter_fields(const vector<field>&f, const vector<link>&l, team_count tc) const
{
    vector<field> fa;

    for (field fi: f) {
        team_count bb;
        for (link li: l) {
            if (fi.intersects(li)) {
                bb.inc_team_enum(li.get_team_enum());
            }
            if (bb > tc)
                break;
        }
        if (!(bb > tc))
            fa.push_back(fi);
                        
                
    }
    return fa;
}

// return only fields that are missing any links in intel
vector<field> field_factory::filter_existing_fields(const vector<field>&f, const vector<link>&l) const
{
    vector<field> ff;

    // Iterate through each field in the fieldArray
    for (field fi : f) {
        int matching = 0;

    // Check if the field contains any of the lines in lineArray
        for (link li : l) {
            if (fi.has_line(li)) {
                matching++;
            }
        }

    // If no lines are found in the field, add it to the filteredFields
        if (matching<3) {
            ff.push_back(fi);
        }
    }

    return ff;

}

// return only fields which have specified cell token
// used in cellfields
vector<field> field_factory::filter_fields_with_cell(const vector<field>&f,string s2cellid_token) const
{
    vector<field> fa;
    for (field fi : f)
    {
        S2Polygon s2p = s2polygon(fi);
        S2CellUnion s2u = cells(s2p);
        for (S2CellId cellid: s2u)
        {
            if (cellid.ToToken() == s2cellid_token)
                fa.push_back(fi);
        }
    }
    return fa;
}

/************************************************
 * MU Functions
*************************************************/

// generates a s2polygon from a field
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

// returns the s2CellUnion (array of cells) for a polygon
S2CellUnion field_factory::cells(const S2Polygon& p) const
{
    S2RegionCoverer::Options opt;
    opt.set_max_level(13);
    opt.set_min_level(0);
    opt.set_max_cells(20);
    S2RegionCoverer rc(opt);

    return rc.GetCovering(p);
}

// calculates the area a polygon overlaps a cell
std::unordered_map<S2CellId,double> field_factory::cell_intersection(const S2Polygon& p) const 
{
    unordered_map<S2CellId,double> area;
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
//        double sqkm = S2Earth::SteradiansToSquareKm(int_poly.GetArea());
        double sqkm = int_poly.GetArea() * point::earth_radius_2;

        //uint64 id = cellid.id();
        //pair<S2CellId,double> area_pair (cellid,sqkm);
        //area->insert(area_pair);
	    area[cellid] = sqkm;

    }

    return area;
}

Json::Value field_factory::json_from_array(const vector<string>& desc) const
{
    Json::Value res;
    if (cell_api.substr(0,4) == "file")
    {
        res = json_reader::read_json_from_file(cell_api);
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
        res = json_reader::read_json_from_http(url);
    }

    return res;
}

unordered_map<string, uniform_distribution>field_factory::query_mu_from_servlet(const vector<string>& cell_tokens) const
{
    unordered_map<string, uniform_distribution> result;
    Json::Value rv = json_from_array(cell_tokens);
    Json::Value def = 0;
    for (string key: rv.getMemberNames())
    {
        double lower = rv.get(key,def)[0].asDouble();
        double upper = rv.get(key,def)[1].asDouble();
        uniform_distribution ud = uniform_distribution(lower,upper);
        result[key]=ud;
    }

    return result;
}

// returns a map of mu values for array of cell tokens
// uses a cell token cache to limit calls to web api
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


uniform_distribution field_factory::calculate_mu(const S2Polygon& p)
{
    // keep swapping back and forward between S2CellId and string (s2cellid tokens)
    unordered_map<S2CellId, double> cell_intersections = cell_intersection(p);
    uniform_distribution total_mu(0.0,0.0);

    // Prepare a list of S2CellIds to query
    S2CellUnion s2u = cells(p);
    vector<string> cell_ids_to_query = union_to_tokens(s2u);

        // Query MU/km2 values from cache
    unordered_map<string, uniform_distribution> mu_values = query_mu(cell_ids_to_query);

        // Calculate total MU
    for (pair<S2CellId, double> entry : cell_intersections) {
        string cell_id = entry.first.ToToken();
        double intersection_area = entry.second;
        if (mu_values.count(cell_id)) {
            total_mu += mu_values[cell_id] * intersection_area;
        }
    }

    return total_mu;

}

int field_factory::get_est_mu(const field& f) 
{
    S2Polygon p = s2polygon(f);
    return (int) calculate_mu(p).rounded_mean();
}

uniform_distribution field_factory::get_ud_mu(const field& f) 
{
    S2Polygon p = s2polygon(f);
    return calculate_mu(p);
}

// returns the mu for a field
int field_factory::get_cache_mu(const field& f)
{
	if (field_mu_cache.count(f))
		return (int)field_mu_cache[f].rounded_mean();

	field_mu_cache[f] = get_ud_mu(f);
	return (int)field_mu_cache[f].rounded_mean();
}

uniform_distribution field_factory::get_cache_ud_mu(const field& f)
{

	if (field_mu_cache.count(f))
		return field_mu_cache[f];

	field_mu_cache[f] = get_ud_mu(f);
	return field_mu_cache[f];
}

vector<string> field_factory::union_to_tokens(const S2CellUnion& s2u) const
{
	vector<string> ctok;
	for (S2CellId s2c : s2u)
		ctok.push_back(s2c.ToToken());

	return ctok;
}

vector<string> field_factory::celltokens(const field& f) const
{
    S2Polygon s2p = s2polygon(f);
	S2CellUnion s2u = cells(s2p);

    return union_to_tokens(s2u);
}

unordered_map<string,double> field_factory::cell_intersection(const field& f) const
{
    S2Polygon s2p = s2polygon(f);
	unordered_map<S2CellId,double> intersections = cell_intersection(s2p);

    unordered_map<string,double> result;
    for (pair<S2CellId,double> ii : intersections)
        result[ii.first.ToToken()] = ii.second;

    return result;
}

/************************************************
 * Field Generation Functions
*************************************************/

/*
// old linear search code
vector<field> field_factory::make_fields_from_single_links(const vector<line>&l) const
{
    vector<field> fa;
    for (int i =0; i<l.size(); i++)
    {
        line l1 = l.at(i);
        for (int j=i+1; j<l.size(); j++)
        {
            line l2 = l.at(j);
            
            // point l1.o == point l2.o
            if (l1.get_o_point() == l2.get_o_point()) {
                if (link_exists(l,j,l1.get_d_point(),l2.get_d_point()))
                    fa.emplace_back(field(l1.get_o_point(),l1.get_d_point(),l2.get_d_point()));
            } else if (l1.o_s2latlng() == l2.d_s2latlng()) {
                if (link_exists(l,j,l1.get_d_point(),l2.get_o_point()))
                    fa.emplace_back(field(l1.get_o_point(),l1.get_d_point(),l2.get_o_point()));
            } else if (l1.get_d_point() == l2.get_o_point()) {
                if (link_exists(l,j,l1.get_o_point(),l2.get_d_point()))
                    fa.emplace_back(field(l1.get_o_point(),l1.get_d_point(),l2.get_d_point()));
            } else if (l1.get_d_point() == l2.get_d_point()) {
                if (link_exists(l,j,l1.get_o_point(),l2.get_o_point()))
                    fa.emplace_back(field(l1.get_o_point(),l1.get_d_point(),l2.get_o_point()));
            }
        }
    }
    return fa;
}
*/

// should be replaced with share_link_index
bool field_factory::link_exists(const vector<line>&l, int j, point p1, point p2) const
{
    for (int k=j+1; k<l.size(); k++)
    {
        line l3 = l.at(k);
        if (
                (p1 == l3.get_o_point() && p2 == l3.get_d_point()) ||
                (p1 == l3.get_d_point() && p2 == l3.get_o_point())
        ) {
                return true;
        }
    }
    return false;

}

// attempted speed up using unordered_set of fields
// support function for field generations functions
bool field_factory::share_line_index(const unordered_map<point, unordered_set<size_t>>& point_exists, const point& p1, const point& p2) const
{
  // Check if both points exist in the map.  If not, they can't share a line.
  //if (point_exists.find(p1) == point_exists.end() || point_exists.find(p2) == point_exists.end()) {
  //  return false;
  //}

  // Get the sets associated with each point.
  const unordered_set<size_t>& set1 = point_exists.at(p1);
  const unordered_set<size_t>& set2 = point_exists.at(p2);

    if (set1.size() < set2.size()) { // iterate the smaller set for performance.
        for (size_t index : set1) {
            if (set2.count(index)) return true;
        }
    } else {
        for (size_t index : set2) {
            if (set1.count(index)) return true;
        }
    }

  return false; // No common index found.
}

vector<field> field_factory::make_fields_from_single_links(const vector<line>& l) const 
{
    unordered_map<point,unordered_set<size_t>> point_exists;

    // Populate the map with existing links
    for (int i = 0; i < l.size(); ++i) {
        const line& li = l[i];
        point_exists[li.get_o_point()].insert(i);
        point_exists[li.get_d_point()].insert(i);
    }

    unordered_set<field> fa;

    for (size_t i = 0; i < l.size(); ++i) {
        const line& l1 = l[i];

        for (size_t k: point_exists[l1.get_o_point()]) {
            if (k > i) {
                const line& l2 = l[k];
                if (!(l1 == l2))
                {
                    // point l1.o == point l2.o
                    if (l1.get_o_point() == l2.get_o_point()) {
                        if (share_line_index(point_exists,l1.get_d_point(),l2.get_d_point())) { // l1.get_d_point() to l2.get_d_point exists
                            fa.insert(field(l1.get_o_point(), l1.get_d_point(), l2.get_d_point()));
                        }
                    } else { // if (l1.get_o_point() == l2.get_d_point()) {
                        if (share_line_index(point_exists,l1.get_d_point(),l2.get_o_point())) { // l1.get_d_point() to l2.get_o_point exists
                            fa.insert(field(l1.get_o_point(), l1.get_d_point(), l2.get_o_point()));
                        }
                    }
                }
            }
        }
    }

    return vector<field>(fa.begin(), fa.end());

}


// the argument order is important.
// two from lines1 and 1 from lines2
/*
vector<field> field_factory::make_fields_from_double_links(const vector<line>&lk1, const vector<line>&lk2) const
{
    vector<field> fa;

    for (int i =0; i<lk1.size(); i++)
    {
        line l1 = lk1.at(i);
        for (int j=i+1; j<lk1.size(); j++)
        {
            line l2 = lk1.at(j);
            
            // point l1.o == point l2.o
            if (l1.get_o_point() == l2.get_o_point()) {
                if (link_exists(lk2,-1,l1.get_d_point(),l2.get_d_point()))
                    fa.push_back(field(l1.get_o_point(),l1.get_d_point(),l2.get_d_point()));
            } else if (l1.get_o_point() == l2.get_d_point()) {
                if (link_exists(lk2,-1,l1.get_d_point(),l2.get_o_point()))
                    fa.push_back(field(l1.get_o_point(),l1.get_d_point(),l2.get_o_point()));
            } else if (l1.get_d_point() == l2.get_o_point()) {
                if (link_exists(lk2,-1,l1.get_o_point(),l2.get_d_point()))
                    fa.push_back(field(l1.get_o_point(),l1.get_d_point(),l2.get_d_point()));
            } else if (l1.get_d_point() == l2.get_d_point()) {
                if (link_exists(lk2,-1,l1.get_o_point(),l2.get_o_point()))
                    fa.push_back(field(l1.get_o_point(),l1.get_d_point(),l2.get_o_point()));
            }
        }
    }   
    return fa;
}
*/

// the argument order is important.
// two from lines1 and 1 from lines2
// optimised version of the make_fields_from_double_links
// makes every possible field from two arrays of links.
vector<field> field_factory::make_fields_from_double_links(const vector<line>& lk1, const vector<line>& lk2) const {
    // Step 1: Index lk1 by point → line index
    unordered_map<point, unordered_set<size_t>> point_index1;
    for (size_t i = 0; i < lk1.size(); ++i) {
        const line& li = lk1[i];
        point_index1[li.get_o_point()].insert(i);
        point_index1[li.get_d_point()].insert(i);
    }

    unordered_map<point, unordered_set<size_t>> point_index2;
    for (size_t i = 0; i < lk2.size(); ++i) {
        const line& li = lk2[i];
        point_index2[li.get_o_point()].insert(i);
        point_index2[li.get_d_point()].insert(i);
    }

    unordered_set<field> field_set;

    // Step 3: Loop through lk1 to build triangles
    for (size_t i = 0; i < lk1.size(); ++i) {
        const line& l1 = lk1[i];

        for (size_t j : point_index1[l1.get_o_point()]) {
            if (j > i) {
                const line& l2 = lk1[j];

                // shared start: l1.o == l2.o
                if (l1.get_o_point() == l2.get_o_point()) {
                    if (share_line_index(point_index2,l1.get_d_point(), l2.get_d_point()))
                        field_set.insert(field(l1.get_o_point(), l1.get_d_point(), l2.get_d_point()));
                }
                else  {
                    if (share_line_index(point_index2,l1.get_d_point(), l2.get_o_point()))
                        field_set.insert(field(l1.get_o_point(), l1.get_d_point(), l2.get_o_point()));
                }
            }
        }
    }

    // Step 4: Convert to vector and return
    return vector<field>(field_set.begin(), field_set.end());
}

// not sure if I should add the optimisations here.
std::vector<field> field_factory::make_fields_from_triple_links(const vector<line>&lk1, const vector<line>&lk2, const vector<line>&lk3) const
{
    vector<field> fa;
    for (int i =0; i<lk1.size(); i++)
    {
        line l1 = lk1.at(i);
        for (int j=0; j<lk2.size(); j++)
        {
            line l2 = lk2.at(j);
            
            // point l1.o == point l2.o
            if (l1.get_o_point() == l2.get_o_point()) {
                if (link_exists(lk3,-1,l1.get_d_point(),l2.get_d_point()))
                    fa.push_back(field(l1.get_o_point(),l1.get_d_point(),l2.get_d_point()));
            } else if (l1.get_o_point() == l2.get_d_point()) {
                if (link_exists(lk3,-1,l1.get_d_point(),l2.get_o_point()))
                    fa.push_back(field(l1.get_o_point(),l1.get_d_point(),l2.get_o_point()));
            } else if (l1.get_d_point() == l2.get_o_point()) {
                if (link_exists(lk3,-1,l1.get_o_point(),l2.get_d_point()))
                    fa.push_back(field(l1.get_o_point(),l1.get_d_point(),l2.get_d_point()));
            } else if (l1.get_d_point() == l2.get_d_point()) {
                if (link_exists(lk3,-1,l1.get_o_point(),l2.get_o_point()))
                    fa.push_back(field(l1.get_o_point(),l1.get_d_point(),l2.get_o_point()));
            }
        }
    }
    return fa;
}

// This function returns the two split fields that would be made when the joining link is made
// If the two supplied fields share no common link this function returns an empty array
vector<field> field_factory::get_splits(const field& f1, const field& f2) const
{
    vector<field> splits;
    if (f1 == f2)
        return splits;
    // find shared line.
    line shared;
    bool found = false;
    for (line l : f1.get_lines())
    {
        for (line l2 : f2.get_lines())
            if (l == l2) 
            {
                found = true;
                shared = l;
                break;
            }
    }
    if (!found)
        return splits;

    point o1 = f1.other_point(shared);
    point o2 = f2.other_point(shared);

    field s1 = field(o1,o2,shared.get_d_point());
    field s2 = field(o1,o2,shared.get_o_point());
    
    splits.push_back(s1);
    splits.push_back(s2);

    return splits;
}

// This function adds all the split fields to an array of fields.
// It compares every field in the array with every other field
// then checks that the split fields for these two fields do not
// intersect any fields in the plan before adding them.
// So should work with any arbitrary number of fields.
vector<field> field_factory::add_splits(const vector<field>& fields) const
{
    unordered_set<field> split_set;
    vector<field> splits_fields;
    for (int i=0; i < fields.size(); i++)
        for (int j=i+1; j < fields.size(); j++)
        {
            vector<field> s = get_splits(fields[i],fields[j]);
            if (s.size() > 0)
            {
                split_set.insert(s[0]);
                split_set.insert(s[1]);
            }
        }
    for (field f : fields)
        splits_fields.push_back(f);

    for (field sf : split_set)
    {
        bool intersects = false;
        for (field pf : splits_fields)
            if (sf.intersects(pf))
            {
                intersects = true;
                break;
            }
        if (!intersects)
            splits_fields.push_back(sf);
    }

    return splits_fields;
}


}
