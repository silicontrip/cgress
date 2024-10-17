#include "run_timer.hpp"
namespace silicontrip {

	void run_timer::start() {
		start_time = high_resolution_clock::now();
		split_time = start_time;
	}

	double run_timer::split() {
		high_resolution_clock::time_point current_time = high_resolution_clock::now();
		std::chrono::duration<double,std::milli> elapsed_time = current_time - split_time;
		split_time = current_time;
		return elapsed_time.count() / 1000.0;
	}

	double run_timer::stop() {
		high_resolution_clock::time_point current_time = high_resolution_clock::now();
		//double elapsed_time = duration_cast<double,seconds>(current_time - start_time).count();
		std::chrono::duration<double,std::milli> elapsed_time = current_time - start_time;
		return elapsed_time.count() / 1000.0;
	}
}
