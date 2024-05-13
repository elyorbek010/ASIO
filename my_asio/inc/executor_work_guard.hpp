#ifndef EXECUTOR_WORK_GUARD_HPP
#define EXECUTOR_WORK_GUARD_HPP

#include "io_context.hpp"

namespace my_asio
{

template<typename Executor>
class executor_work_guard
{
public:
	using executor_type = Executor;

	executor_work_guard(const executor_type& executor)
		: owns_work_(true)
		, executor_(executor)
	{
		executor.on_work_started();
	}

	executor_work_guard(const executor_work_guard& other)
		: owns_work_(other.owns_work_)
		, executor_(other.executor_)
	{ }

	executor_work_guard(executor_work_guard&& other)
		: owns_work_(other.owns_work_)
		, executor_(std::move(other.executor))
	{ }

	~executor_work_guard()
	{
		if (owns_work_)
			executor_.on_work_finished();
	}

	executor_type get_executor() const;

	bool owns_work() const;

	void reset();

private:
	bool owns_work_;
	executor_type executor_;
};

template<typename Executor>
Executor executor_work_guard<Executor>::get_executor() const
{
	return executor_;
}

template<typename Executor>
bool executor_work_guard<Executor>::owns_work() const
{
	return owns_work_;
}

template<typename Executor>
void executor_work_guard<Executor>::reset()
{
	if (owns_work_)
	{
		owns_work_ = false;
		executor_.on_work_finished();
	}
}

} // namespace my_asio

#endif // EXECUTOR_WORK_GUARD_HPP