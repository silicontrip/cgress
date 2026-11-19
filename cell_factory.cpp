#include "cell_factory.hpp"
#include "uniform_distribution.hpp"

#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <cmath>

using namespace std;
namespace fs = std::filesystem;

namespace silicontrip {

cell_factory* cell_factory::ptr = 0;

// --- BilRegion ---

bool cell_factory::BilRegion::covers(double lng, double lat) const
{
    int x = (int)floor((lng - ulxmap) / xdim);
    int y = (int)floor((ulymap - lat) / ydim);
    return x >= 0 && x < ncols && y >= 0 && y < nrows;
}

pair<int,int> cell_factory::BilRegion::toXY(double lng, double lat) const
{
    int x = (int)floor((lng - ulxmap) / xdim);
    int y = (int)floor((ulymap - lat) / ydim);
    return {x, y};
}

void cell_factory::BilRegion::load() const
{
    if (loaded) return;
    ifstream file(bil_path, ios::binary | ios::ate);
    if (!file)
        throw runtime_error("Could not open BIL file: " + bil_path);
    streamsize byteSize = file.tellg();
    file.seekg(0, ios::beg);
    size_t numFloats = byteSize / sizeof(float);
    buffer.resize(numFloats);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), numFloats * sizeof(float)))
        throw runtime_error("Failed to read BIL data: " + bil_path);
    loaded = true;
}

float cell_factory::BilRegion::get_value(int x, int y) const
{
    load();
    return buffer[(size_t)y * ncols + x];
}

// --- Parsing ---

bool cell_factory::parse_hdr(const string& hdr_path, BilRegion& r)
{
    ifstream f(hdr_path);
    if (!f) return false;

    string line;
    while (getline(f, line)) {
        istringstream ss(line);
        string key;
        ss >> key;
        if      (key == "ULXMAP")  ss >> r.ulxmap;
        else if (key == "ULYMAP")  ss >> r.ulymap;
        else if (key == "XDIM")    ss >> r.xdim;
        else if (key == "YDIM")    ss >> r.ydim;
        else if (key == "NCOLS")   ss >> r.ncols;
        else if (key == "NROWS")   ss >> r.nrows;
        else if (key == "NODATA")  ss >> r.nodata;
    }
    return true;
}

// --- Constructor ---

cell_factory::cell_factory()
{
    for (const auto& entry : fs::recursive_directory_iterator("./grump_granules")) {
        if (!entry.is_regular_file()) continue;
        string name = entry.path().filename().string();
        if (name.size() < 10) continue;
        if (name.substr(name.size() - 10) != "up00ag.hdr") continue;

        BilRegion r;
        r.loaded = false;

        if (!parse_hdr(entry.path().string(), r)) {
            cerr << "Warning: could not parse " << entry.path() << "\n";
            continue;
        }

        // derive .bil path from .hdr path
        string hdr = entry.path().string();
        r.bil_path = hdr.substr(0, hdr.size() - 4) + ".bil";

        regions.push_back(move(r));
    }

    if (regions.empty())
        cerr << "Warning: no *up00ag.hdr files found under current directory\n";

    load_exempt("nodata_cells.txt");
}

void cell_factory::load_exempt(const string& path)
{
    ifstream f(path);
    if (!f) return;
    string line;
    while (getline(f, line)) {
        // strip leading/trailing whitespace and skip comments/blanks
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == string::npos || line[start] == '#') continue;
        size_t end = line.find_last_not_of(" \t\r\n");
        exempt_cells.insert(line.substr(start, end - start + 1));
    }
}

bool cell_factory::is_exempt(S2CellId id) const
{
	for (S2CellId cur = id; ; cur = cur.parent()) {
		if (exempt_cells.count(cur.ToToken()))
			return true;
		if (cur.level() == 0)
			break;
	}
	return false;
}

cell_factory* cell_factory::get_instance()
{
    if (!ptr)
        ptr = new cell_factory();
    return ptr;
}

// --- Queries ---

double cell_factory::query_mu(string cellid) const
{
    S2CellId id = S2CellId::FromToken(cellid);

    if (is_exempt(id))
        return 0.0;

    double total = 0.0;
    int count = 0;
    for (S2CellId c = id.child_begin(15); c != id.child_end(15); c = c.next()) {
        S2Cell s2cell = S2Cell(c);
        S2LatLng ll = S2LatLng(s2cell.GetCenter());
        double lat = ll.lat().degrees();
        double lng = ll.lng().degrees();

        const BilRegion* found = nullptr;
        for (const BilRegion& r : regions) {
            if (r.covers(lng, lat)) { found = &r; break; }
        }
        if (!found) {
            ostringstream msg;
            msg << "No BIL region covers lat=" << lat << " lng=" << lng
                << " (from cell " << cellid << ")";
            throw runtime_error(msg.str());
        }

        auto [x, y] = found->toXY(lng, lat);
        total += found->get_value(x, y);
        count++;
    }
    return total / count;
}

unordered_map<string, uniform_distribution> cell_factory::query_mu(const vector<string>& cells) const
{
    unordered_map<string, uniform_distribution> r;
    for (const string& ctok : cells) {
        float cmu = query_mu(ctok);
        r[ctok] = uniform_distribution(cmu - 1e-9, cmu + 1e-9);
    }
    return r;
}

// --- Geometry helpers ---

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

unordered_map<S2CellId,double> cell_factory::cell_intersection(const S2Polygon& p)
{
    unordered_map<S2CellId,double> area;
    S2CellUnion cell_union = cells(p);

    for (S2CellId cellid : cell_union) {
        S2Cell cell = S2Cell(cellid);
        vector<S2Point> cell_points;
        cell_points.push_back(cell.GetVertex(0));
        cell_points.push_back(cell.GetVertex(1));
        cell_points.push_back(cell.GetVertex(2));
        cell_points.push_back(cell.GetVertex(3));
        S2Loop* cell_loop = new S2Loop(cell_points);
        cell_loop->Normalize();
        S2Polygon cell_poly{unique_ptr<S2Loop>(cell_loop)};

        S2Polygon int_poly;
        int_poly.InitToIntersection(p, cell_poly);

        area[cellid] = int_poly.GetArea() * point::earth_radius_2;
    }
    return area;
}

S2CellUnion cell_factory::cells(const S2Polygon& p)
{
    S2RegionCoverer::Options opt;
    opt.set_max_level(13);
    opt.set_min_level(6);
    opt.set_max_cells(20);
    S2RegionCoverer rc(opt);
    return rc.GetCovering(p);
}

uniform_distribution cell_factory::calculate_mu(const S2Polygon& p) const
{
    unordered_map<S2CellId, double> cell_intersections = cell_intersection(p);
    uniform_distribution total_mu(0.0, 0.0);

    S2CellUnion s2u = cells(p);
    vector<string> cell_ids_to_query = union_to_tokens(s2u);
    unordered_map<string, uniform_distribution> mu_values = query_mu(cell_ids_to_query);

    for (pair<S2CellId, double> entry : cell_intersections) {
        string cell_id = entry.first.ToToken();
        double intersection_area = entry.second;
        if (mu_values.count(cell_id))
            total_mu += mu_values[cell_id] * intersection_area;
    }
    return total_mu;
}

uniform_distribution cell_factory::get_ud_mu(const field& f)
{
    return calculate_mu(s2polygon(f));
}

int cell_factory::get_mu(const field& f)
{
	int mu = get_ud_mu(f).rounded_mean();
	if (mu == 0)
		return 1;
	return mu;
}

draw_tools cell_factory::dt_cell(S2CellId s)
{
        S2Cell cell = S2Cell(s);
	vector<point> cv;

	cv.push_back(point(cell.GetVertex(0)));
	cv.push_back(point(cell.GetVertex(1)));
	cv.push_back(point(cell.GetVertex(2)));
	cv.push_back(point(cell.GetVertex(3)));

	draw_tools dt;

	dt.add(cv);

	return dt;

}



}
