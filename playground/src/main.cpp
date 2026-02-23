#include "Job_System.h"
#include "Log.h"
#include "Hash_Set.h"
#include <atomic>

using namespace hstl;

struct Child_Payload {
	uint32_t job_id;
	uint32_t* executed_by_array; // Pointer to a shared array to record who ran this
};

struct Root_Payload {
	Job_System* js;
	uint32_t num_jobs;
	uint32_t* execution_tracker; // Array storing the thread index that ran each job
};

void child_job_fn(void* data) {
	Child_Payload* payload = static_cast<Child_Payload*>(data);

	// Record exactly WHICH thread is executing this job
	payload->executed_by_array[payload->job_id] = t_thread_index;

	// Do some dummy math to keep the thread busy for a microsecond.
	// This guarantees the queue owner doesn't instantly pop everything before thieves arrive.
	volatile float v = 1.0f;
	for (int i = 0; i < 10000; ++i) {
		v *= 1.0001f;
	}
}

void root_job_fn(void* data) {
	Root_Payload* payload = static_cast<Root_Payload*>(data);
	std::atomic<int> counter{ 0 };

	log_info("Root Job (Thread {}): Spawning {} child jobs...", t_thread_index, payload->num_jobs);

	Child_Payload* children = new Child_Payload[payload->num_jobs];

	for (uint32_t i = 0; i < payload->num_jobs; ++i) {
		children[i].job_id = i;
		children[i].executed_by_array = payload->execution_tracker;

		Job child;
		child.function = child_job_fn;
		child.data = &children[i];
		child.parent_counter = &counter;

		payload->js->kick_job(child);
	}

	payload->js->wait(counter);

	delete[] children;
}

int main() {
	Job_System job_system;

	const uint32_t NUM_JOBS = 500;
	uint32_t* execution_tracker = new uint32_t[NUM_JOBS];
	for (uint32_t i = 0; i < NUM_JOBS; ++i) {
		execution_tracker[i] = 9999; // Initialize with a dummy invalid thread index
	}

	std::atomic<int> root_counter{ 0 };

	Root_Payload root_payload;
	root_payload.js = &job_system;
	root_payload.num_jobs = NUM_JOBS;
	root_payload.execution_tracker = execution_tracker;

	Job root_job;
	root_job.function = root_job_fn;
	root_job.data = &root_payload;
	root_job.parent_counter = &root_counter;

	log_info("Main Thread (Thread {}): Kicking Root Job...", t_thread_index);
	job_system.kick_job(root_job);
	job_system.wait(root_counter);

	Hash_Set<uint32_t> unique_threads;
	uint32_t stolen_count = 0;

	uint32_t main_thread_id = t_thread_index;
	for (uint32_t i = 0; i < NUM_JOBS; ++i) {
		uint32_t thread_that_ran_it = execution_tracker[i];
		unique_threads.insert(thread_that_ran_it);

		if (thread_that_ran_it != main_thread_id) {
			stolen_count++;
		}
	}

	log_info("--------------------------------------------------");
	log_info("WORK STEALING REPORT:");
	log_info("Total Jobs Executed: {}", NUM_JOBS);
	log_info("Unique Threads that did work: {}", unique_threads.count());
	log_info("Jobs executed by Main Thread: {}", NUM_JOBS - stolen_count);
	log_info("Jobs STOLEN by Worker Threads: {}", stolen_count);

	if (unique_threads.count() > 1) {
		log_info("SUCCESS: Work stealing is actively happening!");
	}
	else {
		log_warn("FAILURE: Only one thread did all the work. Stealing failed or jobs were too fast.");
	}
	log_info("--------------------------------------------------");

	delete[] execution_tracker;
	return 0;
}