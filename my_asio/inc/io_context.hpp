#ifndef MY_ASIO_IO_CONTEXT_HPP
#define MY_ASIO_IO_CONTEXT_HPP

#include <queue>
#include <functional>
#include <mutex>
#include <atomic>

#include "call_stack.hpp"

namespace my_asio
{

class io_context
{
public:
	class executor_type;
	friend class executor_type;

	io_context(const io_context&) = delete;	
	const io_context& operator=(const io_context&) = delete;

	io_context()
		: stopped_(0)
		, outstanding_work_(0)
	{	}

	~io_context()
	{	}

	executor_type get_executor();

	size_t run();

	size_t run_one();

	size_t poll();

	size_t poll_one();

	void stop();

	bool stopped() const;

	void restart();

private:
	size_t do_one(bool blocking);

	void work_started();

	void work_finished();

	std::atomic<bool> stopped_;
	std::atomic<size_t> outstanding_work_;

	std::mutex queue_guard_;
	std::queue<std::function<void()>> work_queue_;
};

class io_context::executor_type
{
public:
	executor_type(const executor_type& other)
		: io_ptr(other.io_ptr)
	{	}

	executor_type(executor_type&& other)
		: io_ptr(other.io_ptr)
	{
		other.io_ptr = nullptr;
	}

	~executor_type()
	{	}

	executor_type& operator=(const executor_type& executor);
	executor_type& operator=(executor_type&& executor);

	bool running_in_this_thread() const;

	void execute(std::function<void()> f) const;

	io_context& context() const;

	void on_work_started() const;

	void on_work_finished() const;

	bool can_dispatch() const;

	void dispatch(std::function<void()> f) const;

	void post(std::function<void()> f) const;

private:
	friend class io_context;

	executor_type(io_context& io_context)
	{
		io_ptr = &io_context;
	}

	io_context* context_ptr() const
	{
		return io_ptr;
	}

	io_context* io_ptr;
};

void post(io_context& io, std::function<void()> f);

void dispatch(io_context& io, std::function<void()> f);

} // namespace my_asio

#endif // MY_ASIO_IO_CONTEXT_HPP