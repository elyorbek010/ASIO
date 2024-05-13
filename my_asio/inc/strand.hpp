#ifndef STRAND_HPP
#define STRAND_HPP

#include <functional>
#include <atomic>
#include <mutex>
#include <queue>

#include "io_context.hpp"

namespace my_asio
{

template<typename Executor>
class strand
{
public:
	using executor_type = Executor;

	strand(const executor_type& executor)
		: executor_(executor)
		, running_strand(0)
	{	}

	strand(strand&& other)
		: executor_(std::move(other.executor_))
		, running_strand(other.running_strand)
		, work_queue_(std::move(work_queue_))
	{	}

	bool running_in_this_thread();

	void post(std::function<void()> f);

	void dispatch(std::function<void()> f);

private:
	friend executor_type;

	void execute();

	executor_type executor_;
	std::mutex queue_guard_;
	std::atomic<bool> running_strand;
	std::queue<std::function<void()>> work_queue_;
};

template<typename Executor>
void strand<Executor>::execute()
{
	typename detail::call_stack<strand>::context ctx(this);

	std::lock_guard<std::mutex> lock(queue_guard_);
	work_queue_.front()();
	work_queue_.pop();

	if (!work_queue_.empty())
	{
		executor_.post([this]() {
			execute();
			});
		executor_.on_work_finished();
	}
	else
		running_strand = false;
}

template<typename Executor>
bool strand<Executor>::running_in_this_thread()
{
	return detail::call_stack<strand>::contains(this);
}

template<typename Executor>
void strand<Executor>::post(std::function<void()> f)
{
	executor_.on_work_started();
	std::lock_guard<std::mutex> lock(queue_guard_);
	work_queue_.push(f);
	if (!running_strand)
	{
		running_strand = true;
		executor_.post([this]() {
			execute();
			});
		executor_.on_work_finished();
	}
}

template<typename Executor>
void strand<Executor>::dispatch(std::function<void()> f)
{
	if (running_in_this_thread())
		executor_.dispatch(f);
	else
		post(f);
}

template<typename Executor>
void post(strand<Executor>& strand_, std::function<void()> f)
{
	strand_.post(f);
}

template<typename Executor>
void dispatch(strand<Executor>& strand_, std::function<void()> f)
{
	strand_.dispatch(f);
}

} // namespace my_asio

#endif // STRAND_HPP