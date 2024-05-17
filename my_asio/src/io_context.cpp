#include "io_context.hpp"

namespace my_asio
{

size_t io_context::do_one(bool blocking)
{
	for (;;)
	{

		if (stopped())
			return 0;

		std::unique_lock<std::mutex> lock(queue_guard_);

		if (stopped())
		{
			lock.unlock();
			return 0;
		}

		std::function<void()> handler;
		if (!work_queue_.empty())
		{
			handler = work_queue_.front();
			work_queue_.pop();
			lock.unlock(); // unlock before handler so that handler itself could post(acquires lock)
			handler();
			work_finished();
			return 1;
		}
		else if (outstanding_work_)
		{
			if (blocking)
				continue;
		}
		else
		{
			stop();
		}

		return 0;
	}
}

size_t io_context::run()
{
	detail::call_stack<io_context>::context ctx(this);

	size_t cnt = 0;
	while (do_one(1))
		++cnt;

	return cnt;
}

size_t io_context::run_one()
{
	detail::call_stack<io_context>::context ctx(this);

	return do_one(1);
}

size_t io_context::poll()
{
	detail::call_stack<io_context>::context ctx(this);

	size_t cnt = 0;
	while (do_one(0))
		++cnt;

	return cnt;
}

size_t io_context::poll_one()
{
	detail::call_stack<io_context>::context ctx(this);

	return do_one(0);
}

void io_context::stop()
{
	stopped_ = true;
}

bool io_context::stopped() const
{
	return stopped_;
}

void io_context::restart()
{
	stopped_ = false;
}

void io_context::work_started()
{
	outstanding_work_++;
}

void io_context::work_finished()
{
	if (--outstanding_work_ == 0)
		stop();
}

void post(io_context& io, std::function<void()> f)
{
	io.get_executor().post(f);
}

void dispatch(io_context& io, std::function<void()> f)
{
	io.get_executor().dispatch(f);
}

io_context::executor_type io_context::get_executor()
{
	return executor_type(*this);
}

io_context::executor_type& io_context::executor_type::operator=(const io_context::executor_type& other)
{
	io_ptr = other.io_ptr;
	return *this;
}

io_context::executor_type& io_context::executor_type::operator=(io_context::executor_type&& other)
{
	io_ptr = other.io_ptr;
	other.io_ptr = nullptr;
	return *this;
}

bool io_context::executor_type::running_in_this_thread() const
{
	return detail::call_stack<io_context>::contains(io_ptr) != nullptr;
}

void io_context::executor_type::execute(std::function<void()> f) const
{
	f();
}

io_context& io_context::executor_type::context() const
{
	return *context_ptr();
}

void io_context::executor_type::on_work_started() const
{
	io_ptr->work_started();
}

void io_context::executor_type::on_work_finished() const
{
	io_ptr->work_finished();
}

bool io_context::executor_type::can_dispatch() const
{
	if (running_in_this_thread() && !io_ptr->stopped())
		return true;

	return false;
}

void io_context::executor_type::dispatch(std::function<void()> f) const
{
	if (can_dispatch())
		execute(f);
	else
		post(f);
}

void io_context::executor_type::post(std::function<void()> f) const
{
	on_work_started();
	std::lock_guard<std::mutex> lock(io_ptr->queue_guard_);
	io_ptr->work_queue_.push(f);
}

} // namespace my_asio