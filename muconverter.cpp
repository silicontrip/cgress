#include <fstream>
#include <iostream>
#include <vector>

#include <s2/s2cell.h>
#include <s2/s2cell_id.h>
#include <s2/s2latlng.h>
#include <s2/s2region.h>



#include "arguments.hpp"
#include "run_timer.hpp"

using namespace std;
using namespace silicontrip;


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


S2LatLng fromXY(int x, int y)
{

	float lat = -8.0 - y * YDIM;
	float lng = 96.0 + x * XDIM;

	return S2LatLng::FromDegrees(lat,lng);

}

pair<int,int> toXY(S2LatLng ll)
{
	// int x = (ll.lng().degrees() - 96.0) / 0.0083333333333;
	// int y = (-8.0 - ll.lat().degrees()) / 0.0083333333333;

	double lat = ll.lat().degrees();
	double lng = ll.lng().degrees();

	int x = (int) floor((lng - 96.0041666666666) * 120.0);
	int y = (int) floor((-8.00416666681065 - lat) * 120.0);

	return pair(x,y);
}

int main (int argc, char* argv[]) 
{

	arguments ag(argc,argv);
	run_timer rt;


        if (!ag.parse())
        {
                cerr << "muconvert <bil file> <cell id>" << endl;
                exit(1);
        }

        if (ag.argument_size() < 2)
        {
                cerr << "muconvert <bil file> <cell id>" << endl;
                exit(1);
        }



	string filename = ag.get_argument_at(0);

	std::ifstream file(filename, std::ios::binary | std::ios::ate);
    
	if (!file) {
		std::cerr << "Error: Could not open file " << filename << "\n";
		return 1;
	}

	std::streamsize byteSize = file.tellg();
	file.seekg(0, std::ios::beg);

	std::size_t numFloats = byteSize / sizeof(float);
	std::vector<float> buffer(numFloats);

        cerr << "== Reading GRUMP data ==" << endl;
        rt.start();

	if (!file.read(reinterpret_cast<char*>(buffer.data()), numFloats * sizeof(float))) {
		std::cerr << "Error: Failed to read complete binary data.\n";
		return 1;
	}

	cerr << "== read completed in "<< rt.split() << " seconds. ==" << endl;


	string cellid = ag.get_argument_at(1);


	S2CellId id = S2CellId::FromToken(cellid);


	float total = 0.0;
	for(S2CellId c = id.child_begin(15); c != id.child_end(15); c = c.next())
	{
		S2Cell s2cell = S2Cell(c);
		S2Point s2point = s2cell.GetCenter();
		S2LatLng s2latlng = S2LatLng(s2point);	

		pair<int,int> arr = toXY(s2latlng);

		int x = arr.first;
		int y = arr.second;

		int stride = y * NCOLS + x;

		cout << c.ToToken() << ": " << s2latlng << " : " << arr.first <<", " << arr.second << " : " << buffer[stride] << endl;

		total += buffer[stride];

	}

	cout << "Ave: " << std::setprecision(12) << (total / 16.0) << endl;
	double error = (6371.01 / 6367.0) * (6371.01 / 6367.0);
//	cout << "Ave adjusted: " << std::setprecision(12) << (total / 16.0) * error << endl;


        cerr <<  "== Finished. " << rt.split() << " elapsed time. " << rt.stop() << " total time." << endl;


}
