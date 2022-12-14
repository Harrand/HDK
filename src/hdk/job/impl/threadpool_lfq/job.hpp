#ifndef HDK_JOB_IMPL_THREADPOOL_LFQ_JOB_HPP
#define HDK_JOB_IMPL_THREADPOOL_LFQ_JOB_HPP
#include "hdk/job/api/job.hpp"
#include "concurrentqueue.h"
#include <deque>
#include <thread>
#include <memory>
#include <mutex>
#include <condition_variable>

namespace hdk::impl
{
	class job_system_threadpool_lfq
	{
	public:
		job_system_threadpool_lfq();
		~job_system_threadpool_lfq();

		job_handle execute(job_t job);
		void block(job_handle j) const;
		bool complete(job_handle j) const;
		bool any_work_remaining() const;
		void block_all() const;
		unsigned int size() const;
	private:
		struct worker_t
		{
			std::thread thread;
			std::size_t local_tid;
			std::atomic<std::size_t> current_job;
		};
		struct job_info_t
		{
			job_t func;
			std::size_t id;
		};
		void tmain(std::size_t local_tid);
		static void wait_a_bit();

		std::deque<worker_t> thread_pool;
		moodycamel::ConcurrentQueue<job_info_t> jobs;
		std::atomic<bool> requires_exit = false;
		std::atomic<std::size_t> lifetime_count = 0;
		std::vector<std::size_t> waiting_job_ids = {};
		mutable std::mutex waiting_job_id_mutex;
		mutable std::mutex wake_mutex;
		mutable std::condition_variable wake_condition;
	};

	static_assert(job_system_type<job_system_threadpool_lfq>);
}

#endif // HDK_JOB_IMPL_THREADPOOL_LFQ_JOB_HPP
