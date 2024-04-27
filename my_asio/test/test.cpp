#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <catch2/catch_test_macros.hpp>

#include "io_context.hpp"
#include <boost/asio.hpp>

TEST_CASE("executor_work_guard reset when out of scope", "[executor_work_guard][io_context][io_context::run]")
{
	/*
	Thread calling run() is blocked as long as executor_work_guard is not destroyed
	*/
	my_asio::io_context io;
	std::atomic<bool> starting_run(false);
	std::atomic<bool> finished_run(false);

	{
		my_asio::executor_work_guard work(io);

		REQUIRE(work.owns_work() == true);

		std::thread([&]() {
			starting_run = true;
		
			io.run();

			finished_run = true;
			}).detach();

		std::this_thread::sleep_for(std::chrono::milliseconds(10));

		REQUIRE(starting_run == true);
		REQUIRE(finished_run == false);

		io.run();

		std::this_thread::sleep_for(std::chrono::milliseconds(10));

		REQUIRE(finished_run == false);
	
		REQUIRE(work.owns_work() == true);
	} // executor_work_guard object is out of scope

	std::this_thread::sleep_for(std::chrono::milliseconds(10));

	REQUIRE(finished_run == true);
}

TEST_CASE("executor_work_guard manual reset", "[executor_work_guard][executor_work_guard::owns_work][[executor_work_guard::reset][io_context][io_context::run]")
{
	/*
	Thread calling run() is blocked as long as executor_work_guard is not reset
	*/
	my_asio::io_context io;
	std::atomic<bool> starting_run(false);
	std::atomic<bool> finished_run(false);

	my_asio::executor_work_guard work(io);

	REQUIRE(work.owns_work() == true);

	std::thread([&]() {
		starting_run = true;

		io.run();

		finished_run = true;
		}).detach();

	std::this_thread::sleep_for(std::chrono::milliseconds(10));

	REQUIRE(starting_run == true);
	REQUIRE(finished_run == false);

	io.run();

	std::this_thread::sleep_for(std::chrono::milliseconds(10));

	REQUIRE(finished_run == false);

	REQUIRE(work.owns_work() == true);
	work.reset();
	REQUIRE(work.owns_work() == false);

	std::this_thread::sleep_for(std::chrono::milliseconds(10));

	REQUIRE(finished_run == true);
}

TEST_CASE("post 1 work", "[post][io_context][io_context::run]")
{
	/*
	When io_context::run is called, the variabled "checked" is set to 'true' and the run exits
	*/
	my_asio::io_context io;
	bool checked(false);

	my_asio::post(io, [&checked]() {
		checked = true;
		});

	REQUIRE(checked == false);

	io.run();

	REQUIRE(checked == true);
}

TEST_CASE("post several works", "[post][io_context][io_context::run]")
{
	constexpr int NUMBER_OF_WORKS = 10;

	my_asio::io_context io;
	int counter(0);

	for(int i = 0; i != NUMBER_OF_WORKS; ++i)
		my_asio::post(io, [&counter]() {
			counter++;
			});

	REQUIRE(counter == 0);

	io.run();

	REQUIRE(counter == NUMBER_OF_WORKS);
}

TEST_CASE("post from several threads", "[post][io_context][io_context::run]")
{
	constexpr int NUMBER_OF_THREADS = 10;

	my_asio::io_context io;
	std::atomic<int> counter(0);

	for (int i = 0; i != NUMBER_OF_THREADS; ++i)
		std::thread(my_asio::post(io, [&counter]() {
		counter++;
			})).detach();

	REQUIRE(counter == 0);

	io.run();

	REQUIRE(counter == NUMBER_OF_THREADS);
}

TEST_CASE("run async_op", "[io_context][io_context::run][io_context::executor_type][io_context::get_executor][io_context::executor_type::on_work_started][io_context::executor_type::on_work_finished][post]")
{
	/*
	io_context::run() should block and run event processing loop until no outstanding work and ready handlers left
	*/
	constexpr int NUMBER_OF_WORKS = 10;

	my_asio::io_context io;
	int counter(0);

	for (int i = 0; i != NUMBER_OF_WORKS; ++i)
		io.get_executor().on_work_started(); // simulate that async_op is being done

	auto t = std::thread([&io]() {
		io.run();
		});

	std::this_thread::sleep_for(std::chrono::milliseconds(10));
	REQUIRE(t.joinable() == false);

	for (int i = 0; i != NUMBER_OF_WORKS; ++i)
	{
		my_asio::post(io, [&counter]() {
			counter++;
			});

		io.get_executor().on_work_finished();
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(10));

	REQUIRE(counter == NUMBER_OF_WORKS);

	REQUIRE(t.joinable() == true);
	t.join();
}

TEST_CASE("run_one asynch_op", "[io_context][io_context::run_one][io_context::executor_type][io_context::get_executor][io_context::executor_type::on_work_started][io_context::executor_type::on_work_finished][post]")
{
	/*
	io_context::run_one() should block if outstanding work exists and returns immediately after executing one handler
	*/
	constexpr int NUMBER_OF_WORKS = 10;

	my_asio::io_context io;
	int counter(0);

	for (int i = 0; i != NUMBER_OF_WORKS; ++i)
		io.get_executor().on_work_started(); // simulate that async_op is being done

	auto t = std::thread([&io]() {
		io.run_one();
		});

	std::this_thread::sleep_for(std::chrono::milliseconds(10));
	REQUIRE(t.joinable() == false);

	for (int i = 0; i != NUMBER_OF_WORKS; ++i)
	{
		my_asio::post(io, [&counter]() {
			counter++;
			});

		io.get_executor().on_work_finished();
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(10));

	REQUIRE(counter == 1);

	REQUIRE(t.joinable() == true);
	t.join();
}

// poll poll one
TEST_CASE("poll", "[io_context][io_context::poll][io_context::executor_type][io_context::get_executor][io_context::executor_type::on_work_started][io_context::executor_type::on_work_finished][post]")
{
	/*
	io_context::poll() executes ready handlers, if no ready handlers available - returns
	*/
	constexpr int NUMBER_OF_WORKS = 10;

	my_asio::io_context io;
	int counter(0);

	for (int i = 0; i != NUMBER_OF_WORKS; ++i)
		io.get_executor().on_work_started(); // simulate that async_op is being done

	auto t1 = std::thread([&io]() {
		io.poll();
		});

	std::this_thread::sleep_for(std::chrono::milliseconds(10));
	REQUIRE(t1.joinable() == true);
	t1.join();

	for (int i = 0; i != NUMBER_OF_WORKS; ++i)
	{
		my_asio::post(io, [&counter]() {
			counter++;
			});

		io.get_executor().on_work_finished();
	}

	auto t2 = std::thread([&io]() {
		io.poll();
		});

	std::this_thread::sleep_for(std::chrono::milliseconds(10));
	REQUIRE(t2.joinable() == true);
	t2.join();

	REQUIRE(counter == NUMBER_OF_WORKS);
}

TEST_CASE("poll one", "[io_context][io_context::poll_one][io_context::executor_type][io_context::get_executor][io_context::executor_type::on_work_started][io_context::executor_type::on_work_finished][post]")
{
	/*
	io_context::poll_one() executes one ready handler if available, if not - returns
	*/
	constexpr int NUMBER_OF_WORKS = 10;

	my_asio::io_context io;
	int counter(0);

	for (int i = 0; i != NUMBER_OF_WORKS; ++i)
		io.get_executor().on_work_started(); // simulate that async_op is being done

	auto t1 = std::thread([&io]() {
		io.poll_one();
		});

	std::this_thread::sleep_for(std::chrono::milliseconds(10));
	REQUIRE(t1.joinable() == true);
	t1.join();

	for (int i = 0; i != NUMBER_OF_WORKS; ++i)
	{
		my_asio::post(io, [&counter]() {
			counter++;
			});

		io.get_executor().on_work_finished();
	}

	auto t2 = std::thread([&io]() {
		io.poll_one();
		});

	std::this_thread::sleep_for(std::chrono::milliseconds(10));
	REQUIRE(t2.joinable() == true);
	t2.join();

	REQUIRE(counter == 1);
}


TEST_CASE("stop/restart io", "[io_context][io_context::run][io_context::run_one][io_context::poll][io_context::poll_one][io_context::post][io_context::stop][io_context::stopped][io_context::restart]")
{
	/*
	When io.stop() is called, run/run_one/poll/poll_one should return immediately
	*/
	constexpr int NUMBER_OF_WORKS = 1'000'000'000;

	my_asio::io_context io;
	std::atomic<int> counter(0);
	std::atomic<bool> process(true);

	for (int i = 0; i != NUMBER_OF_WORKS; ++i)
		my_asio::post(io, [&counter]() {
		counter++;
			});

	REQUIRE(counter == 0);

	SECTION("run")
	{
		std::thread([&io, &process]() {
			while (process)
				io.run(); 
			}).detach();
	}
	SECTION("run_one")
	{
		std::thread([&io, &process]() {
			while (process)
				io.run_one();
			}).detach();
	}
	SECTION("poll")
	{
		std::thread([&io, &process]() {
			while (process)
				io.poll();
			}).detach();
	}
	SECTION("poll_one")
	{
		std::thread([&io, &process]() {
			while (process)
				io.poll_one();
			}).detach();
	}
	
	std::this_thread::sleep_for(std::chrono::nanoseconds(500));

	io.stop();

	int saved_counter = counter;

	REQUIRE(counter > 0 && counter < NUMBER_OF_WORKS);
	std::this_thread::sleep_for(std::chrono::milliseconds(10));
	REQUIRE(saved_counter == counter);

	REQUIRE(io.stopped() == true);

	io.restart();

	std::this_thread::sleep_for(std::chrono::seconds(1));

	REQUIRE(counter == NUMBER_OF_WORKS);
}

TEST_CASE("strand run", "[io_context][io_context::run][strand][post]")
{
	/*
	handlers posted through strand are performed synchronously, never concurrently
	in this test s_counter should never be subject to a race condition since it is never incremented concurrently
	also, all strand handlers must be executed
	*/
	my_asio::io_context io;
	strand<my_asio::io_context::executor_type> strand_(io);
	std::atomic<int> a_counter(0);
	int s_counter(0);

	constexpr int NUMBER_OF_WORKS = 100;

	for (int i = 0; i != NUMBER_OF_WORKS; ++i)
		my_asio::post(strand_, [&s_counter]() {
		s_counter++;
			});

	constexpr int NUMBER_OF_WORKERS = 100;

	for (int i = 0; i != NUMBER_OF_WORKERS; ++i)
		std::thread([]() {
			io.run();
			}).detach();

	std::this_thread::sleep_for(std::chrono::milliseconds(10));

	REQUIRE(s_counter == NUMBER_OF_WORKS);
}

TEST_CASE("strand run_one", "[io_context][io_context::run_one][strand][post]")
{
	/*
	handlers posted through strand are performed synchronously, never concurrently
	in this test s_counter should never be subject to a race condition since it is never incremented concurrently
	also, all strand handlers must be executed since NUMBER_OF_WORKS == NUMBER_OF_WORKERS
	*/
	my_asio::io_context io;
	my_asio::strand<my_asio::io_context::executor_type> strand_(io);
	std::atomic<int> a_counter(0);
	int s_counter(0);

	constexpr int NUMBER_OF_WORKS = 100;

	for (int i = 0; i != NUMBER_OF_WORKS; ++i)
		my_asio::post(strand_, [&s_counter]() {
		s_counter++;
			});

	constexpr int NUMBER_OF_WORKERS = 100;

	for (int i = 0; i != NUMBER_OF_WORKERS; ++i)
		std::thread([]() {
			io.run_one();
			}).detach();

	std::this_thread::sleep_for(std::chrono::milliseconds(10));

	REQUIRE(s_counter == NUMBER_OF_WORKERS);
}

TEST_CASE("strand poll", "[io_context][io_context::poll][strand][post]")
{
	/*
	handlers posted through strand are performed synchronously, never concurrently
	in this test s_counter should never be subject to a race condition since it is never incremented concurrently
	also, all strand handlers must be executed
	*/
	my_asio::io_context io;
	strand<my_asio::io_context::executor_type> strand_(io);
	std::atomic<int> a_counter(0);
	int s_counter(0);

	constexpr int NUMBER_OF_WORKS = 1000;

	for (int i = 0; i != NUMBER_OF_WORKS; ++i)
		my_asio::post(strand_, [&s_counter]() {
		s_counter++;
			});

	constexpr int NUMBER_OF_WORKERS = 10;

	for (int i = 0; i != NUMBER_OF_WORKERS; ++i)
		std::thread([]() {
			io.poll();
			}).detach();

	std::this_thread::sleep_for(std::chrono::milliseconds(10));

	REQUIRE(s_counter == NUMBER_OF_WORKS);
}

TEST_CASE("strand poll_one", "[io_context][io_context::poll_one][strand][post]")
{
	/*
	all the handlers must be executed since all work is already posted and is ready, and the NUMBER_OF_WORKS == NUMBER_OF_WORKERS
	*/
	my_asio::io_context io;
	strand<my_asio::io_context::executor_type> strand_(io);
	std::atomic<int> a_counter(0);
	int s_counter(0);

	constexpr int NUMBER_OF_WORKS = 100;

	for (int i = 0; i != NUMBER_OF_WORKS; ++i)
		my_asio::post(strand_, [&s_counter]() {
		s_counter++;
			});

	constexpr int NUMBER_OF_WORKERS = 100;

	for (int i = 0; i != NUMBER_OF_WORKERS; ++i)
		std::thread([&io]() {
			io.poll_one();
			}).detach();

	std::this_thread::sleep_for(std::chrono::milliseconds(10));

	REQUIRE(s_counter >= 1);
}

TEST_CASE("dispatch while running", "[io_context][io_context::run][dispatch][post]")
{
	/*
	dispatch executes the handler immediately if running in this thread
	*/
	my_asio::io_context io;
	std::atomic<int> counter(0);
	std::mutex m;
	std::condition_variable cv;

	bool dispatched(false);
	
	for (int i = 0; i < 100; i++)
	{
		my_asio::post(io, [&io, &counter, &cv, &m, &dispatched]() {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			counter++;
			if (counter == 50)
			{
				my_asio::dispatch(io, [&m, &cv, &dispatched]() {
					std::lock_guard<std::mutex> lock(m);
					dispatched = true;
					cv.notify_one();
					});
			}
			});

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	auto t = std::thread([&io]() { io.run(); });

	std::unique_lock<std::mutex> lock(m);
	cv.wait(lock, [&dispatched]() { return dispatched; });
	
	if (counter >= 50 && counter < 100)
		REQUIRE(true);
	else
		REQUIRE(false);

	t.join();
}

TEST_CASE("dispatch while run_one", "[io_context][io_context::run_one][dispatch][post]")
{
	my_asio::io_context io;
	std::atomic<int> dispatched(false);
	std::atomic<int> counter(0);

	for (int i = 0; i != 100; ++i)
		my_asio::post(io, [&]() {
			if (i == 0)
				my_asio::dispatch(io, [&dispatched]() {dispatched = true; });
			counter++;
			});

	io.run_one();

	REQUIRE(counter == 1);
	REQUIRE(dispatched == true);
}

TEST_CASE("dispatch while polling", "[io_context][io_context::poll][dispatch][post]")
{
	/*
	dispatch executes the handler immediately if running in this thread
	*/
	my_asio::io_context io;
	std::atomic<int> counter(0);
	std::mutex m;
	std::condition_variable cv;

	bool dispatched(false);

	for (int i = 0; i < 100; i++)
	{
		my_asio::post(io, [&io, &counter, &cv, &m, &dispatched]() {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			counter++;
			if (counter == 50)
			{
				my_asio::dispatch(io, [&m, &cv, &dispatched]() {
					std::lock_guard<std::mutex> lock(m);
					dispatched = true; 
					cv.notify_one(); 
					});
			}
			});

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	auto t = std::thread([&io]() { io.poll(); });

	std::unique_lock<std::mutex> lock(m);
	cv.wait(lock, [&dispatched]() { return dispatched; });

	if (counter >= 50 && counter < 100)
		REQUIRE(true);
	else
		REQUIRE(false);

	t.join();
}

TEST_CASE("dispatch while poll_one", "[io_context][io_context::poll_one][dispatch][post]")
{
	my_asio::io_context io;
	std::atomic<int> dispatched(false);
	std::atomic<int> counter(0);

	for (int i = 0; i != 100; ++i)
		my_asio::post(io, [&]() {
			if (i == 0)
				my_asio::dispatch(io, [&dispatched]() {dispatched = true; });
			counter++;
			});

	io.poll_one();

	REQUIRE(counter == 1);
	REQUIRE(dispatched == true);
}