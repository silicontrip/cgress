#include "cell_factory.hpp"
#include "uniform_distribution.hpp"

using namespace std;


// update for your bil data

#define FILE "ausup00g/ausup00ag.bil"

#define BYTEORDER      I
#define LAYOUT         BIL
#define NROWS          4320
#define NCOLS          7680
#define NBANDS         1
#define NBITS          32
#define BANDROWBYTES   30720
#define TOTALROWBYTES  30720
#define PIXELTYPE      FLOAT
#define ULXMAP         96.0041666666666
#define ULYMAP         -8.00416666681065
#define XDIM           0.0083333333333
#define YDIM           0.0083333333333
#define NODATA         -3.4028235e+038


namespace silicontrip {
cell_factory* cell_factory::ptr = 0;

cell_factory::cell_factory()
{

	string filename = FILE;

        std::ifstream file(filename, std::ios::binary | std::ios::ate);

        if (!file) {
                std::cerr << "Error: Could not open file " << filename << "\n";
        }

        std::streamsize byteSize = file.tellg();
        file.seekg(0, std::ios::beg);

        std::size_t numFloats = byteSize / sizeof(float);
        buffer = std::vector<float>(numFloats);

        if (!file.read(reinterpret_cast<char*>(buffer.data()), numFloats * sizeof(float))) {
                std::cerr << "Error: Failed to read complete binary data.\n";
        }

}

cell_factory* cell_factory::get_instance()
{
    if (!ptr)
	    ptr = new cell_factory();
    return ptr;
}

pair<int,int> cell_factory::toXY(S2LatLng ll) 
{

        double lat = ll.lat().degrees();
        double lng = ll.lng().degrees();


	// 120 = 1 / XDIM
	// 120 = 1 / YDIM
        int x = (int) floor((lng - ULXMAP) * 120.0);
        int y = (int) floor((ULYMAP - lat) * 120.0);

        return pair(x,y);
}


double cell_factory::query_mu(string cellid) const
{
        S2CellId id = S2CellId::FromToken(cellid);

        double total = 0.0;
        int count = 0;
        for(S2CellId c = id.child_begin(15); c != id.child_end(15); c = c.next())
        {       
                S2Cell s2cell = S2Cell(c);
                S2Point s2point = s2cell.GetCenter();
                S2LatLng s2latlng = S2LatLng(s2point);
        
                pair<int,int> arr = toXY(s2latlng);
        
                int x = arr.first;
                int y = arr.second;
        
                int stride = y * NCOLS + x; 

                total += buffer[stride];
                count ++;

        }
        return total / count;


}

// arg code duplication!
// generates a s2polygon from a field
S2Polygon cell_factory::s2polygon(const field& f)
{
    vector<S2Point> loop_points;
    loop_points.push_back(f.point_at(0).s2latlng().ToPoint());
    loop_points.push_back(f.point_at(1).s2latlng().ToPoint());
    loop_points.push_back(f.point_at(2).s2latlng().ToPoint());

    S2Loop* field_loop = new S2Loop(loop_points);
    field_loop->Normalize();

    return S2Polygon(unique_ptr<S2Loop>(field_loop));
}

vector<string> cell_factory::union_to_tokens(const S2CellUnion& s2u)
{
        vector<string> ctok;
        for (S2CellId s2c : s2u)
                ctok.push_back(s2c.ToToken());

        return ctok;
}


std::unordered_map<S2CellId,double> cell_factory::cell_intersection(const S2Polygon& p)
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

        double sqkm = int_poly.GetArea() * point::earth_radius_2;

	area[cellid] = sqkm;

    }

    return area;
}

S2CellUnion cell_factory::cells(const S2Polygon& p)
{
    S2RegionCoverer::Options opt;
    opt.set_max_level(13);
    opt.set_min_level(0);
    opt.set_max_cells(20);
    S2RegionCoverer rc(opt);

    return rc.GetCovering(p);
}


uniform_distribution cell_factory::calculate_mu(const S2Polygon& p) const
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

uniform_distribution cell_factory::get_ud_mu(const field& f)
{
    S2Polygon p = s2polygon(f);
    return calculate_mu(p);
}


int cell_factory::get_mu(const field& f)
{
	return (int)get_ud_mu(f).rounded_mean();
}

unordered_map<string, uniform_distribution> cell_factory::query_mu(const vector<string>& cells) const
{
	unordered_map<std::string, uniform_distribution> r;
	for (const std::string& ctok: cells)
	{
		float cmu = query_mu(ctok);
		uniform_distribution ud(cmu-1e-7, cmu+1e-7);
		r[ctok] = ud;
	}
	return r;
}	

}
