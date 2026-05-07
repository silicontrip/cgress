#include <Foundation/Foundation.h>
#import <MetalKit/MetalKit.h>
#include <objc/NSObjCRuntime.h>
#import <Metal/Metal.h>

#include <cstdint>
#include <random>

#include "portal.hpp"
#include "field.hpp"
#include "draw_tools.hpp"
#include "arguments.hpp"
#include "run_timer.hpp"
#include "portal_factory.hpp"
#include "link_factory.hpp"
#include "field_factory.hpp"

using namespace silicontrip;
using namespace std;

struct thread_state {                                                                                                 
	float    temperature;                                                                                           
	uint32_t iterations;
	uint32_t score;         // best score seen this run
	uint32_t _pad;                                                                                                    
};

struct global_state {
	uint32_t field_count;
	uint32_t buffer_size;
	uint32_t seed;
};

@interface Field : NSObject {

	id<MTLDevice> mDevice;
    id<MTLLibrary> mDefaultLibrary;
    id<MTLFunction> mFunction;
    id<MTLComputePipelineState> mPSO;
    id<MTLCommandQueue> mCommandQueue;

	unsigned int threads;
	uint32_t fieldCount;
	uint32_t bytesPerBlock;
	uint32_t bytesPerRow;


	uint32_t best;
	float bestsub;
	@public
	vector<field> fields;
	int calc_type;

	std::random_device rd;

}



- (id<MTLBuffer>)cliqueBufferWithFields:(vector<field>&)f;

@end

double calculate_balance_score(const vector<field>& fi)
{
	unordered_map<point, int> link_counts;

	for (field fd : fi) {
		vector<point> portals = fd.get_points();
		for (point portal : portals) {
			if (link_counts.find(portal) == link_counts.end())
				link_counts[portal]= 1;
			else
				link_counts[portal]= link_counts[portal] + 1;
		}
	}

	int totalLinks = 0;
	int totalPortals = link_counts.size();
	for (pair<point,int> count : link_counts) {
		totalLinks += count.second;
	}

	double mean = (double) totalLinks / totalPortals;
	double variance = 0.0;

	for (pair<point,int> count : link_counts) {
		variance += pow(count.second - mean, 2);
	}

	variance /= totalPortals;
	return sqrt(variance); // return the standard deviation as the balance score
}

double get_geo_value ( const vector<field>& fd)
{
	field_factory* ff = field_factory::get_instance();
	double total = 0;

	for (field f : fd)
		total += f.geo_area();

	return total;
}

double get_mu_value (const vector<field>& fd)
{
	field_factory* ff = field_factory::get_instance();
	double total = 0;

	for (field f : fd)
		total += ff->get_cache_mu(f);

	return total;
}


@implementation Field

- (instancetype)init {

	self=[super init];
	if (self)
	{
		mDevice = MTLCreateSystemDefaultDevice();
		if (!mDevice) {
			NSLog(@"Metal is not supported on this device");
			return nil;
		}

		mDefaultLibrary = [mDevice newDefaultLibrary];
		if (!mDefaultLibrary) {
			NSLog(@"Failed to load default Metal library");
			return nil;
		}

		mFunction = [mDefaultLibrary newFunctionWithName:@"field_anneal"];
		if (!mFunction) {
			NSLog(@"Failed to find metal function");
			return nil;
		}

		NSError *err = nil;
		mPSO = [mDevice newComputePipelineStateWithFunction:mFunction error:&err];
		if (err) {
			NSLog(@"PSO Error: %@", err);
			return nil;
		}

		mCommandQueue = [mDevice newCommandQueue];
		if (!mCommandQueue)
		{
			NSLog(@"Failed to initialise command queue");
			return nil;
		}

		NSLog(@"Initialised Metal thread execution width: %lu max threads per threadgroup: %lu", 
	      mPSO.threadExecutionWidth, mPSO.maxTotalThreadsPerThreadgroup);

		threads = mPSO.maxTotalThreadsPerThreadgroup;

	}

	return self;
}

- (id<MTLBuffer>)resultBufferWithLen:(uint32_t)l
{
	uint32_t bytesNeeded = (l + 7) / 8;
	uint32_t bsize = (bytesNeeded + 15) & ~15u;  // round up to 16-byte boundary
	bytesPerBlock = bsize + sizeof(struct thread_state);

	uint32_t fsize = bytesPerBlock * threads;

	id<MTLBuffer> resultBuffer = [mDevice newBufferWithLength:fsize
	                                        options:MTLResourceStorageModeShared];

	return resultBuffer;
}

- (id<MTLBuffer>)cliqueBufferWithFields:(vector<field>&)all_fields
{

	fieldCount = all_fields.size();

	bytesPerRow = (fieldCount + 7) / 8;
	uint32_t mlen = fieldCount * bytesPerRow;

	id<MTLBuffer> cliqueBuffer = [mDevice newBufferWithLength:mlen 
	                                        options:MTLResourceStorageModeShared];

	uint8_t* clique = (uint8_t*)[cliqueBuffer contents];
	//memset(clique, 0xFF, mlen);

	for (int i=0; i<fieldCount; i++)
	{
		for (int j=i+1; j<fieldCount; j++)
		{
			bool c = all_fields[i].intersects(all_fields[j]);
			if (!c) // If they intersect, they are NOT compatible
			{
				uint32_t byte;
				uint8_t bit;

				byte = bytesPerRow * i + (j / 8);
				bit = 1 << (7 - (j % 8));
				clique[byte] = clique[byte] | bit;

				byte = bytesPerRow * j + (i / 8);
				bit = 1 << (7 - (i % 8));
				clique[byte] = clique[byte] | bit;
			}
		}
	}
	return cliqueBuffer;
}

- (NSArray*)remainingWith:(uint8_t*)buffer atOffset:(uint32_t)offset withCliqueBuffer:(uint8_t*)clique
{
	// get list of fields in use
	NSMutableArray* fieldList = [[NSMutableArray alloc] init];

	for (uint32_t i=0; i < fieldCount; i++)
	{
		uint32_t byte = i / 8;
		uint32_t bit = 1 << (7 - (i % 8));

		bool fi = buffer[offset+byte] & bit;
		if (fi != 0)
			[fieldList addObject:@(i)];
	}

	uint32_t listCount = [fieldList count];

	//NSLog(@"remainingWith: listCount: %lu",listCount);

	if (listCount > 0)
	{
		// generate remaining fields.
		NSMutableArray* remainList = [[NSMutableArray alloc] init];

		for (uint32_t i=0; i < fieldCount; i++)
		{
			BOOL allMatch = YES;
			uint32_t fieldOffset = i * bytesPerRow;

			for (uint32_t j=0; j < listCount; j++)
			{
				uint32_t fn = [[fieldList objectAtIndex:j] intValue];
				if (fn == i)
				{
					allMatch = NO;
					break;
				}
				uint32_t byte = fn / 8;
				uint32_t bit = 1 << (7 - (fn % 8));
				bool fi = clique[fieldOffset+byte] & bit;
				if (fi == 0)
				{
					allMatch = NO;
					break;
				}

			}
			if (allMatch)
				[remainList addObject:@(i)];
		}
		//NSLog(@"remainingWith: remainListCount: %lu",[remainList count]);

		return [remainList copy];
	}
	return nil;
} 

- (NSInteger) countBuffer:(uint8_t*)buffer atOffset:(uint32_t)offset
{
	NSInteger count = 0;
	for (uint32_t i=0; i < fieldCount; i++)
	{
		uint32_t byte = i / 8;
		uint32_t bit = 1 << (7 - (i % 8));

		bool fi = buffer[offset+byte] & bit;
		if (fi != 0)
			count ++;
	}
	return count;
}

- (void)randomiseBuffer:(id<MTLBuffer>)resultsBuffer withCliqueBuffer:(id<MTLBuffer>)cliqueBuffer
{
	mt19937 gen(rd());

	uint8_t* buffer = (uint8_t*)[resultsBuffer contents];
	uint8_t* clique = (uint8_t*)[cliqueBuffer contents];
	uint32_t bpr = bytesPerRow; // bytes per clique row = (fieldCount+7)/8

	for (uint32_t t = 0; t < threads; t++)
	{
		uint8_t* sol = buffer + bytesPerBlock * t + sizeof(struct thread_state);
		memset(sol, 0, bpr);

		// pick a random starting field
		uniform_int_distribution<uint32_t> dist(0, fieldCount - 1);
		uint32_t f = dist(gen);
		sol[f / 8] |= 1 << (7 - (f % 8));

		// intersect the compatible-candidates mask with each added field's clique row
		// compatible[] tracks which fields can still be added
		vector<uint8_t> compatible(bpr, 0xFF);
		// clear bits beyond fieldCount
		uint32_t tail = fieldCount % 8;
		if (tail) compatible[bpr - 1] = (0xFF << (8 - tail)) & 0xFF;

		// remove f from candidates and intersect with f's clique row
		compatible[f / 8] &= ~(1 << (7 - (f % 8)));
		uint8_t* clique_f = clique + f * bpr;
		for (uint32_t b = 0; b < bpr; b++)
			compatible[b] &= clique_f[b];

		while (true)
		{
			// collect candidate indices from compatible[]
			vector<uint32_t> cands;
			for (uint32_t i = 0; i < fieldCount; i++)
				if (compatible[i / 8] & (1 << (7 - (i % 8))))
					cands.push_back(i);

			if (cands.empty()) break;

			uniform_int_distribution<uint32_t> pick(0, (uint32_t)cands.size() - 1);
			uint32_t next = cands[pick(gen)];

			sol[next / 8] |= 1 << (7 - (next % 8));

			// remove next from candidates and intersect with next's clique row
			compatible[next / 8] &= ~(1 << (7 - (next % 8)));
			uint8_t* clique_next = clique + next * bpr;
			for (uint32_t b = 0; b < bpr; b++)
				compatible[b] &= clique_next[b];
		}
	}
}

- (void) initHeader:(id<MTLBuffer>)resultBuffer iterations:(uint32_t)it temperature:(float)temp
{
	uint8_t* buffer = (uint8_t*)[resultBuffer contents];
	for (uint32_t i=0; i < threads; i++)
	{
		struct thread_state* ts = (struct thread_state*)(i * bytesPerBlock + buffer);

		ts->iterations = it;
		ts->temperature = temp;
		ts->score = [self countBuffer:buffer atOffset:i * bytesPerBlock + sizeof(struct thread_state)];
	}

}

- (uint32_t) bestFromBuffer:(id<MTLBuffer>)resultBuffer at:(uint32_t)i
{
	uint8_t* buffer = (uint8_t*)[resultBuffer contents];
	struct thread_state* ts = (struct thread_state*)(i * bytesPerBlock + buffer);
	return ts->score;
}

- (double)subscore:(id<MTLBuffer>)resultBuffer at:(uint32_t)i
{
	if (calc_type == 0)
		return 0.0;
	uint8_t* buffer = (uint8_t*)[resultBuffer contents];
	uint64_t offset = (i * bytesPerBlock + sizeof(struct thread_state));
	vector<field>fd;
	for (uint32_t j=0; j < fieldCount; j++)
	{
	
		uint32_t byte = j / 8;
		uint32_t bit = 1 << (7 - (j % 8));

		bool fi = buffer[offset+byte] & bit;
		if (fi != 0)
			fd.push_back(fields[j]);
	
	}

	if (calc_type == 1)
		return get_mu_value(fd);
	if (calc_type == 2)
		return get_geo_value(fd);
	if (calc_type == 3)
		return calculate_balance_score(fd);
	if (calc_type == 4)
		return 10.0 - calculate_balance_score(fd);

	return 0.0;
}

- (void)displayPlan:(id<MTLBuffer>)resultBuffer at:(uint32_t)i
{
	uint8_t* buffer = (uint8_t*)[resultBuffer contents];
	uint64_t offset = (i * bytesPerBlock + sizeof(struct thread_state));
	draw_tools dt;
	for (uint32_t j=0; j < fieldCount; j++)
	{
	
		uint32_t byte = j / 8;
		uint32_t bit = 1 << (7 - (j % 8));

		bool fi = buffer[offset+byte] & bit;
		if (fi != 0)
			dt.add(fields[j]);
	
	}
	cout << dt.to_string() << endl << endl;

	
}

- (void)bestSearch:(id<MTLBuffer>)resultBuffer
{
	for (uint32_t i = 0; i < threads; i++)
	{
		uint32_t tbest = [self bestFromBuffer:resultBuffer at:i];
		if (tbest == best && calc_type != 0)
		{

			double tsub = [self subscore:resultBuffer at:i];
			
			if (tsub - bestsub > 1e-5)
			{
				bestsub = tsub;
				NSLog(@"%u + %f",best,bestsub);
				[self displayPlan:resultBuffer at:i];
			}
		}
		if (tbest > best)
		{
			bestsub = [self subscore:resultBuffer at:i];
			best = tbest;
			//display plan
			if (calc_type != 0)
				NSLog(@"%u + %f",best,bestsub);
			else
				NSLog(@"%u",best);
			[self displayPlan:resultBuffer at:i];

		}
	}
}

- (void)runWithBuffer:(id<MTLBuffer>)resultBuffer clique:(id<MTLBuffer>)cliqueBuffer
{

	best = 0;
	float globalTemp = 2.0;

	struct global_state gs = {fieldCount, bytesPerBlock, (uint32_t)rd()};
	[self initHeader:resultBuffer iterations:20000 temperature:globalTemp];
	[self bestSearch:resultBuffer];


	while (globalTemp > 1e-6)
	{
		//NSLog(@"Temperature: %f",globalTemp);



		id<MTLCommandBuffer> commandBuffer = [mCommandQueue commandBuffer];
		id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
		[encoder setComputePipelineState:mPSO];
		[encoder setBytes:&gs length:sizeof(struct global_state) atIndex:0];
		[encoder setBuffer:cliqueBuffer offset:0 atIndex:1];
		[encoder setBuffer:resultBuffer offset:0 atIndex:2];

		NSUInteger tgSize = mPSO.threadExecutionWidth;  // 32 on Apple Silicon
		NSUInteger tgCount = (threads + tgSize - 1) / tgSize;
		MTLSize threadgroupSize = MTLSizeMake(tgSize, 1, 1);
		MTLSize gridSize = MTLSizeMake(tgCount, 1, 1);
		[encoder dispatchThreadgroups:gridSize threadsPerThreadgroup:threadgroupSize];
		[encoder endEncoding];

		CFAbsoluteTime t0 = CFAbsoluteTimeGetCurrent();
		[commandBuffer commit];
		[commandBuffer waitUntilCompleted];
		CFAbsoluteTime t1 = CFAbsoluteTimeGetCurrent();

		if (commandBuffer.error)
			NSLog(@"GPU error: %@  status: %ld", commandBuffer.error, (long)commandBuffer.status);
		//NSLog(@"GPU dispatch: %.1f ms", (t1 - t0) * 1000);

		//CFAbsoluteTime t2 = CFAbsoluteTimeGetCurrent();
		[self bestSearch:resultBuffer];
		//CFAbsoluteTime t3 = CFAbsoluteTimeGetCurrent();
		//NSLog(@"bestSearch: %.1f ms", (t3 - t2) * 1000);

		globalTemp *= 0.95;
		[self initHeader:resultBuffer iterations:20000 temperature:globalTemp];

	}
}

@end

void field_metal(vector<field>& fields, int calc) 
{
	Field* mv = [[Field alloc] init];
	if (mv)
	{
		NSLog(@"generating clique buffer: %lu",fields.size());
		id<MTLBuffer> fc = [mv cliqueBufferWithFields:fields];
		NSLog(@"done");
		id<MTLBuffer> rb = [mv resultBufferWithLen:fields.size()];

		// random init
		NSLog(@"randomising");
		[mv randomiseBuffer:rb withCliqueBuffer:fc];
		NSLog(@"done");

		mv->fields = fields;
		mv->calc_type = calc;

		[mv runWithBuffer:rb clique:fc ];

	}

}

bool geo_comparison(const field& a, const field& b)
{
    return a.geo_area() > b.geo_area();
}

vector<portal> cluster_and_filter_from_description(const vector<portal>& remove, const string desc)
{
	portal_factory* pf = portal_factory::get_instance();
    vector<portal> portals = pf->cluster_from_description(desc);
    if (remove.size() > 0)
        portals = pf->remove_portals(portals, remove);
    return portals;
}

vector<line> filter_lines (const vector<line>& li, const vector<silicontrip::link>& links, const team_count& tc, const vector<portal>& avoid_double, bool limit2k, double percentile)
{
	link_factory* lf = link_factory::get_instance();
    vector<line> la = lf->filter_links(li, links, tc);
    if (avoid_double.size() > 0)
        la = lf->filter_link_by_blocker(la, links, avoid_double);

    if (limit2k)
        la = lf->filter_link_by_length(la, 2);

    if (percentile < 100)
        la = lf->percentile_lines(la, percentile);

    return la;
}

void print_usage()
{
		cerr << "Usage:" << endl;
		cerr << "maxfields [options] <portal cluster> [<portal cluster> [<portal cluster>]]" << endl;
		cerr << "    if two clusters are specified, 2 portals are chosen to make links in the first cluster." << endl;
		cerr << "Generates the maximum number of fields possible for a given portal cluster descriptions." << endl;
		cerr << "Options:" << endl;
		cerr << " -E <number>       Limit number of Enlightened Blockers" << endl;
		cerr << " -R <number>       Limit number of Resistance Blockers" << endl;
		cerr << " -N <number>       Limit number of Machina Blockers" << endl;
		cerr << " -D <cluster>      Filter links crossing blockers using these portals" << endl;
		cerr << " -a <cluster>      Avoid linking to these portals" << endl;
		cerr << " -i <cluster>      Ignore blocking links from these portals" << endl;
		cerr << " -k                Limit links to 2km" << endl;
		cerr << " -r <drawtools>    Remove these fields from the plan" << endl;


		cerr << " -C <#colour>      Set Drawtools output colour" << endl;
		cerr << " -L                Set Drawtools to output as polylines" << endl;
		cerr << " -I                Output as Intel Link" << endl;
		cerr << " -s				Display plans that have the same size as the best found with decreasing variance" << endl;
		cerr << " -S				Same as -s but with increasing variance (can't use with -s)" << endl;
		cerr << " -M                Use MU calculation" << endl;
		cerr << " -x <MU>           Target exactly <MU> amount" << endl;
		cerr << " -l <number>       Limit maximum number of fields" << endl;
		cerr << " -T <lat,lng,...>  Use only fields covering target points" << endl;
}

int main (int argc, char* argv[])
{
	run_timer rt;

	vector<point>target;
	int calc = 0;  // area or mu

	vector<portal>avoid_double;
	vector<portal>avoid_single;
	vector<portal>ignore_links;
	bool limit2k = false;
	double percentile = 100;
	long target_mu = -1;
	int max_fields = 0;

	arguments ag(argc,argv);

	ag.add_req("E","enlightened",true); // max enlightened blockers
	ag.add_req("R","resistance",true); // max resistance blockers
	ag.add_req("N","machina",true); // max machina blockers
	ag.add_req("i","ignore",true); // ignore links from these portals (about to decay or easy to destroy)
	ag.add_req("a","avoid", true); // avoid using these portals.  S in other tools
	ag.add_req("k","limit2k",false); // limit link length to that can be made under fields.
	ag.add_req("p","lpercent",true); // use percentile longest links
	ag.add_req("D","blockers",true); // remove links with blocker using these portals.


	ag.add_req("C","colour",true); // drawtools colour
	ag.add_req("I","intel",false); // output as intel
	ag.add_req("L","polyline",false); // output as polylines
	ag.add_req("M","MU",false); // calculate as MU
	ag.add_req("S","same",false); // display same size plans
	ag.add_req("s","samesmall",false); // display same size plans
	ag.add_req("G","geo",false); // display same size plans
	ag.add_req("x","target_mu",true); // Target MU
	ag.add_req("l","limit",true); // Limit fields
	ag.add_req("r","remove",true); // remove fields based on drawtools.

	ag.add_req("T","target",true); // target fields over location
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
	if (ag.has_option("I"))
		dt.set_output_as_intel();



	if (ag.has_option("k"))
		limit2k=true;

	if (ag.has_option("p"))
		percentile = ag.get_option_for_key_as_double("p");

	if (ag.has_option("x")) {
		calc = 1; // Force MU calculation
		target_mu = ag.get_option_for_key_as_int("x"); // Assuming int handles long enough for MU? 
		// get_option_for_key_as_int returns int. MU could be large. 
		// I should check arguments.hpp if there is as_long or just use atol(get_option_for_key).
		target_mu = atol(ag.get_option_for_key("x").c_str());
	}

	if (ag.has_option("l"))
		max_fields = ag.get_option_for_key_as_int("l");

	portal_factory* pf = portal_factory::get_instance();
	link_factory* lf = link_factory::get_instance();
	field_factory* ff = field_factory::get_instance();

	if (ag.has_option("M"))
		calc = 1;
	if (ag.has_option("G"))
	{
		calc = 2;
	}
	if (ag.has_option("s"))
		calc = 3;

	if (ag.has_option("S"))
	{
		calc = 4;
	}


	if (ag.has_option("T"))
		target = pf->points_from_string(ag.get_option_for_key("T"));

	if (ag.has_option("D"))
		avoid_double = pf->cluster_from_description(ag.get_option_for_key("D"));

	if (ag.has_option("a"))
		avoid_single = pf->cluster_from_description(ag.get_option_for_key("a"));

	cerr << "== Reading links and portals ==" << endl;
	rt.start();

// of course I had to pick a colliding name for my class
	vector<silicontrip::link> links;
	vector<field> all_fields;
	vector<vector<portal>> clusters;
	vector<portal> all_portals;

	//vector<field> af;

	try {if (ag.argument_size() == 1) {
        clusters.push_back(cluster_and_filter_from_description(avoid_single, ag.get_argument_at(0)));
    } else if (ag.argument_size() == 2) {
        clusters.push_back(cluster_and_filter_from_description(avoid_single, ag.get_argument_at(0)));
        clusters.push_back(cluster_and_filter_from_description(avoid_single, ag.get_argument_at(1)));
    } else if (ag.argument_size() == 3) {
        clusters.push_back(cluster_and_filter_from_description(avoid_single, ag.get_argument_at(0)));
        clusters.push_back(cluster_and_filter_from_description(avoid_single, ag.get_argument_at(1)));
        clusters.push_back(cluster_and_filter_from_description(avoid_single, ag.get_argument_at(2)));
    } else {
        print_usage();
        exit(1);
    }

  for (const vector<portal>& cluster : clusters) {
        all_portals.insert(all_portals.end(), cluster.begin(), cluster.end());
    }

    cerr << "== " << all_portals.size() << " portals read. in " << rt.split() << " seconds. ==" << endl;
    cerr << "== getting links ==" << endl;

	    // Get purged links
    vector<silicontrip::link> links = lf->get_purged_links(all_portals);
	if (!ignore_links.empty())
		links = lf->filter_link_by_portal(links,ignore_links);
    cerr <<  "== " << links.size() << " links read. in " << rt.split() <<  " seconds ==" << endl;
    cerr << "== generating potential links ==" << endl;

	if (ag.argument_size() == 1) {
        vector<line> li = lf->make_lines_from_single_cluster(clusters[0]);
        cerr << "all links: " << li.size() << endl;

        li = filter_lines(li, links, tc, avoid_double, limit2k, percentile);

        cerr << "== " << li.size() << " links generated " << rt.split() << " seconds. Generating fields ==" << endl;

        all_fields = ff->make_fields_from_single_links(li);
    } else if (ag.argument_size() == 2) {
        vector<line> li1 = filter_lines(lf->make_lines_from_single_cluster(clusters[0]), links, tc, avoid_double, limit2k, percentile);
        cerr << "== cluster 1 links:  " << li1.size() << " ==" << endl;

        vector<line> li2 = filter_lines(lf->make_lines_from_double_cluster(clusters[0], clusters[1]), links, tc, avoid_double, limit2k, percentile);
        cerr << "== cluster 2 links:  " << li2.size() << " ==" << endl;

        all_fields = ff->make_fields_from_double_links(li2, li1);
    } else if (ag.argument_size() == 3) {
        vector<line> li1 = filter_lines(lf->make_lines_from_double_cluster(clusters[0], clusters[1]), links, tc, avoid_double, limit2k, percentile);
        cerr << "== cluster 1 links:  " << li1.size() << " ==" << endl;

        vector<line> li2 = filter_lines(lf->make_lines_from_double_cluster(clusters[1], clusters[2]), links, tc, avoid_double, limit2k, percentile);
        cerr << "== cluster 2 links:  " << li2.size() << " ==" << endl;

        vector<line> li3 = filter_lines(lf->make_lines_from_double_cluster(clusters[2], clusters[0]), links, tc, avoid_double, limit2k, percentile);
        cerr << "== cluster 3 links:  " << li3.size() << " ==" << endl;

        all_fields = ff->make_fields_from_triple_links(li1, li2, li3);
    }

	cerr << "== " << all_fields.size() << " fields generated " << rt.split() << " seconds ==" << endl;

    // Common field processing

	if (ag.has_option("E") || ag.has_option("R"))
	    all_fields = ff->filter_existing_fields(all_fields, links); // need to make this optional. Or related to team blockers.

    all_fields = ff->filter_fields(all_fields, links, tc);

	if (!target.empty())
	{
		all_fields = ff->over_target(all_fields,target);
	}

	cerr << "== " << all_fields.size() << " fields filtered " << rt.split() << " seconds ==" << endl;
	if (all_fields.size() == 0)
	{
		cerr << "No fields remaining after filtering" << endl;
		exit(1);
	}
	cerr << "== sorting fields ==" << endl;

	sort(all_fields.begin(),all_fields.end(),geo_comparison);

	cerr << "==  fields sortered " << rt.split() << " ==" << endl;

	cerr << "== show matches ==" << endl;


	field_metal(all_fields,calc);


	cerr <<  "== Finished. " << rt.split() << " elapsed time. " << rt.stop() << " total time." << endl;

	} catch (exception &e) {
		cerr << "An Error occured: " << e.what() << endl;
	}

	return 0;
}
