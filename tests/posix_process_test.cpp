/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voß <thomas.voss@canonical.com>
 */

#include <core/posix/exec.h>
#include <core/posix/fork.h>
#include <core/posix/process.h>
#include <core/posix/signal.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <map>
#include <thread>

namespace
{
::testing::AssertionResult is_error(const std::error_code& ec)
{
    return ec ? ::testing::AssertionResult{true} : ::testing::AssertionResult{false};
}

struct ForkedSpinningProcess : public ::testing::Test
{
    void SetUp()
    {
        child = core::posix::fork(
                    []() { std::cout << "Child" << std::endl; while(true) {} return core::posix::exit::Status::failure;},
                    core::posix::StandardStream::stdin | core::posix::StandardStream::stdout);
    }

    void TearDown()
    {
    }

    core::posix::ChildProcess child = core::posix::ChildProcess::invalid();
};
}

TEST(PosixProcess, ctor_throws_for_invalid_pid)
{
    pid_t invalid_pid{-1};
    EXPECT_ANY_THROW(core::posix::Process{invalid_pid});
}

TEST(PosixProcess, this_process_instance_reports_correct_pid)
{
    EXPECT_EQ(getpid(), core::posix::this_process::instance().pid());
}

TEST(PosixProcess, this_process_instance_reports_correct_parent)
{
    EXPECT_EQ(getppid(), core::posix::this_process::parent().pid());
}

TEST(PosixProcess, throwing_access_to_process_group_id_of_this_process_works)
{
    EXPECT_EQ(getpgrp(), core::posix::this_process::instance().process_group_or_throw().id());
}

TEST(PosixProcess, non_throwing_access_to_process_group_id_of_this_process_works)
{
    std::error_code se;
    auto pg = core::posix::this_process::instance().process_group(se);
    EXPECT_FALSE(is_error(se));
    EXPECT_EQ(getpgrp(), pg.id());
}

TEST(PosixProcess, trying_to_access_process_group_of_invalid_process_throws)
{
    EXPECT_ANY_THROW(core::posix::Process::invalid().process_group_or_throw());
}

TEST(PosixProcess, trying_to_access_process_group_of_invalid_process_reports_error)
{
    std::error_code se;
    core::posix::Process::invalid().process_group(se);
    EXPECT_TRUE(is_error(se));
}

TEST_F(ForkedSpinningProcess, throwing_access_to_process_group_id_of_a_forked_process_works)
{
    auto pg = child.process_group_or_throw();
    EXPECT_EQ(getpgrp(), pg.id());
}

TEST_F(ForkedSpinningProcess, non_throwing_access_to_process_group_id_of_a_forked_process_works)
{
    std::error_code se;
    auto pg = child.process_group(se);

    EXPECT_FALSE(se);
    EXPECT_EQ(getpgrp(), pg.id());
}

TEST(PosixProcess, accessing_streams_of_this_process_works)
{
    {
        std::stringstream ss;
        auto old = core::posix::this_process::cout().rdbuf(ss.rdbuf());
        core::posix::this_process::cout() << "core::posix::this_process::instance().cout()\n";
        EXPECT_EQ(ss.str(), "core::posix::this_process::instance().cout()\n");
        core::posix::this_process::cout().rdbuf(old);
    }

    {
        std::stringstream ss;
        auto old = core::posix::this_process::cerr().rdbuf(ss.rdbuf());
        core::posix::this_process::cerr() << "core::posix::this_process::instance().cerr()" << std::endl;
        EXPECT_EQ(ss.str(), "core::posix::this_process::instance().cerr()\n");
        core::posix::this_process::cerr().rdbuf(old);
    }
}

TEST(Self, non_mutable_access_to_the_environment_returns_correct_results)
{
    static const char* home = "HOME";
    static const char* totally_not_existent = "totally_not_existent_42";
    EXPECT_EQ(getenv("HOME"), core::posix::this_process::env::get(home));
    EXPECT_EQ("", core::posix::this_process::env::get(totally_not_existent));
}

TEST(Self, mutable_access_to_the_environment_alters_the_environment)
{
    static const char* totally_not_existent = "totally_not_existent_42";
    static const char* totally_not_existent_value = "42";

    EXPECT_EQ("", core::posix::this_process::env::get(totally_not_existent));
    EXPECT_NO_THROW(core::posix::this_process::env::set_or_throw(
                        totally_not_existent,
                        totally_not_existent_value));
    EXPECT_EQ(totally_not_existent_value,
              core::posix::this_process::env::get(totally_not_existent));

    EXPECT_NO_THROW(
                core::posix::this_process::env::unset_or_throw(
                    totally_not_existent));
    EXPECT_EQ("",
              core::posix::this_process::env::get(totally_not_existent));
}

TEST(Self, getting_env_var_for_empty_key_does_not_throw)
{
    EXPECT_NO_THROW(core::posix::this_process::env::get(""));
}

TEST(Self, setting_env_var_for_empty_key_throws)
{
    EXPECT_ANY_THROW(core::posix::this_process::env::set_or_throw(
                        "",
                        "uninteresting"));
}

TEST(ChildProcess, fork_returns_process_object_with_valid_pid_and_wait_for_returns_correct_result)
{
    core::posix::ChildProcess child = core::posix::fork(
                []() { std::cout << "Child" << std::endl; return core::posix::exit::Status::success; },
                core::posix::StandardStream::stdin | core::posix::StandardStream::stdout);
    EXPECT_TRUE(child.pid() > 0);

    auto result = child.wait_for(core::posix::wait::Flags::untraced);
    EXPECT_EQ(core::posix::wait::Result::Status::exited,
              result.status);
    EXPECT_EQ(core::posix::exit::Status::success,
              result.detail.if_exited.status);

    child = core::posix::fork(
                []() { std::cout << "Child" << std::endl; return core::posix::exit::Status::failure; },
                core::posix::StandardStream::stdin | core::posix::StandardStream::stdout);
    EXPECT_TRUE(child.pid() > 0);

    result = child.wait_for(core::posix::wait::Flags::untraced);
    EXPECT_EQ(core::posix::wait::Result::Status::exited,
              result.status);
    EXPECT_EQ(core::posix::exit::Status::failure,
              result.detail.if_exited.status);
}

TEST_F(ForkedSpinningProcess, signalling_a_forked_child_makes_wait_for_return_correct_result)
{
    EXPECT_NO_THROW(child.send_signal_or_throw(core::posix::Signal::sig_kill));
    auto result = child.wait_for(core::posix::wait::Flags::untraced);
    EXPECT_EQ(core::posix::wait::Result::Status::signaled,
              result.status);
    EXPECT_EQ(core::posix::Signal::sig_kill,
              result.detail.if_signaled.signal);

    child = core::posix::fork(
                []() { std::cout << "Child" << std::endl; while(true) {} return core::posix::exit::Status::failure;},
                core::posix::StandardStream::stdin | core::posix::StandardStream::stdout);
    EXPECT_TRUE(child.pid() > 0);

    EXPECT_NO_THROW(child.send_signal_or_throw(core::posix::Signal::sig_term));
    result = child.wait_for(core::posix::wait::Flags::untraced);
    EXPECT_EQ(core::posix::wait::Result::Status::signaled,
              result.status);
    EXPECT_EQ(core::posix::Signal::sig_term,
              result.detail.if_signaled.signal);
}

TEST(ChildProcess, stopping_a_forked_child_makes_wait_for_return_correct_result)
{
    core::posix::ChildProcess child = core::posix::fork(
                []()
                {
                    std::string line;
                    while(true)
                    {
                        std::cin >> line;
                        std::cout << line << std::endl;
                    }
                    return core::posix::exit::Status::failure;
                },
                core::posix::StandardStream::stdin | core::posix::StandardStream::stdout);
    EXPECT_TRUE(child.pid() > 0);

    const std::string echo_value{"42"};
    child.cin() << echo_value << std::endl;
    std::string line; child.cout() >> line;

    EXPECT_EQ(echo_value, line);

    EXPECT_NO_THROW(child.send_signal_or_throw(core::posix::Signal::sig_stop));
    auto result = child.wait_for(core::posix::wait::Flags::untraced);
    EXPECT_EQ(core::posix::wait::Result::Status::stopped,
              result.status);
    EXPECT_EQ(core::posix::Signal::sig_stop,
              result.detail.if_stopped.signal);

    EXPECT_NO_THROW(child.send_signal_or_throw(core::posix::Signal::sig_kill));
    result = child.wait_for(core::posix::wait::Flags::untraced);
    EXPECT_EQ(core::posix::wait::Result::Status::signaled,
              result.status);
    EXPECT_EQ(core::posix::Signal::sig_kill,
              result.detail.if_signaled.signal);
}

TEST(ChildProcess, exec_returns_process_object_with_valid_pid_and_wait_for_returns_correct_result)
{
    const std::string program{"/usr/bin/sleep"};
    const std::vector<std::string> argv = {"10"};
    std::map<std::string, std::string> env;
    core::posix::this_process::env::for_each([&env](const std::string& key, const std::string& value)
    {
        env.insert(std::make_pair(key, value));
    });

    core::posix::ChildProcess child = core::posix::exec(program,
                                            argv,
                                            env,
                                            core::posix::StandardStream::stdin | core::posix::StandardStream::stdout);
    EXPECT_TRUE(child.pid() > 0);
    EXPECT_NO_THROW(child.send_signal_or_throw(core::posix::Signal::sig_kill));
    auto result = child.wait_for(core::posix::wait::Flags::untraced);
    EXPECT_EQ(core::posix::wait::Result::Status::signaled,
              result.status);
    EXPECT_EQ(core::posix::Signal::sig_kill,
              result.detail.if_signaled.signal);
}

TEST(ChildProcess, signalling_an_execd_child_makes_wait_for_return_correct_result)
{
    const std::string program{"/usr/bin/env"};
    const std::vector<std::string> argv = {};
    std::map<std::string, std::string> env;
    core::posix::this_process::env::for_each([&env](const std::string& key, const std::string& value)
    {
        env.insert(std::make_pair(key, value));
    });

    core::posix::ChildProcess child = core::posix::exec(
                program,
                argv,
                env,
                core::posix::StandardStream::stdin | core::posix::StandardStream::stdout);

    EXPECT_TRUE(child.pid() > 0);

    EXPECT_NO_THROW(child.send_signal_or_throw(core::posix::Signal::sig_kill));
    auto result = child.wait_for(core::posix::wait::Flags::untraced);
    EXPECT_EQ(core::posix::wait::Result::Status::signaled,
              result.status);
    EXPECT_EQ(core::posix::Signal::sig_kill,
              result.detail.if_signaled.signal);

    child = core::posix::exec(program,
                        argv,
                        env,
                        core::posix::StandardStream::stdin | core::posix::StandardStream::stdout);
    EXPECT_TRUE(child.pid() > 0);

    EXPECT_NO_THROW(child.send_signal_or_throw(core::posix::Signal::sig_term));
    result = child.wait_for(core::posix::wait::Flags::untraced);
    EXPECT_EQ(core::posix::wait::Result::Status::signaled,
              result.status);
    EXPECT_EQ(core::posix::Signal::sig_term,
              result.detail.if_signaled.signal);
}

TEST(ChildProcess, stopping_an_execd_child_makes_wait_for_return_correct_result)
{
    const std::string program{"/usr/bin/sleep"};
    const std::vector<std::string> argv = {"10"};
    std::map<std::string, std::string> env;
    core::posix::this_process::env::for_each([&env](const std::string& key, const std::string& value)
    {
        env.insert(std::make_pair(key, value));
    });

    core::posix::ChildProcess child = core::posix::exec(program,
                                            argv,
                                            env,
                                            core::posix::StandardStream::stdin | core::posix::StandardStream::stdout);
    EXPECT_TRUE(child.pid() > 0);

    EXPECT_NO_THROW(child.send_signal_or_throw(core::posix::Signal::sig_stop));
    auto result = child.wait_for(core::posix::wait::Flags::untraced);
    EXPECT_EQ(core::posix::wait::Result::Status::stopped,
              result.status);
    EXPECT_EQ(core::posix::Signal::sig_stop,
              result.detail.if_signaled.signal);
    EXPECT_NO_THROW(child.send_signal_or_throw(core::posix::Signal::sig_kill));
    result = child.wait_for(core::posix::wait::Flags::untraced);
    EXPECT_EQ(core::posix::wait::Result::Status::signaled,
              result.status);
    EXPECT_EQ(core::posix::Signal::sig_kill,
              result.detail.if_signaled.signal);
}

namespace
{
struct ChildDeathObserverSignalTrap
{
    MOCK_METHOD1(on_child_died,void(const core::posix::Process&));
};
}

TEST_F(ForkedSpinningProcess, observing_child_processes_for_death_works_if_child_is_signalled_with_sigkill)
{
    using namespace ::testing;

    ChildDeathObserverSignalTrap signal_trap;

    auto& death_observer = core::posix::ChildProcess::DeathObserver::instance();

    EXPECT_TRUE(death_observer.add(child));

    core::ScopedConnection sc
    {
        death_observer.child_died().connect([&signal_trap](const core::posix::ChildProcess& child)
        {
            signal_trap.on_child_died(child);
        })
    };

    EXPECT_CALL(signal_trap, on_child_died(_))
            .Times(1)
            .WillOnce(
                InvokeWithoutArgs(
                    &death_observer,
                    &core::posix::ChildProcess::DeathObserver::quit));

    std::error_code ec;
    std::thread worker{[&death_observer, &ec]() { death_observer.run(ec); }};

    child.send_signal_or_throw(core::posix::Signal::sig_kill);

    if (worker.joinable())
        worker.join();

    EXPECT_FALSE(ec);
}

TEST_F(ForkedSpinningProcess, observing_child_processes_for_death_works_if_child_is_signalled_with_sigterm)
{
    using namespace ::testing;

    ChildDeathObserverSignalTrap signal_trap;

    auto& death_observer = core::posix::ChildProcess::DeathObserver::instance();

    EXPECT_TRUE(death_observer.add(child));

    core::ScopedConnection sc
    {
        death_observer.child_died().connect([&signal_trap](const core::posix::ChildProcess& child)
        {
            signal_trap.on_child_died(child);
        })
    };

    EXPECT_CALL(signal_trap, on_child_died(_))
            .Times(1)
            .WillOnce(
                InvokeWithoutArgs(
                    &death_observer,
                    &core::posix::ChildProcess::DeathObserver::quit));

    std::error_code ec;
    std::thread worker{[&death_observer, &ec]() { death_observer.run(ec); }};

    child.send_signal_or_throw(core::posix::Signal::sig_term);

    if (worker.joinable())
        worker.join();

    EXPECT_FALSE(ec);
}

TEST(ChildProcess, ensure_that_forked_children_are_cleaned_up)
{
    static const unsigned int child_process_count = 100;
    static unsigned int counter = 0;

    auto& death_observer = core::posix::ChildProcess::DeathObserver::instance();
    core::ScopedConnection sc
    {
        death_observer.child_died().connect([&death_observer](const core::posix::ChildProcess&)
        {
            counter++;

            if (counter == child_process_count)
            {
                death_observer.quit();
            }
        })
    };

    std::error_code ec;
    std::thread t([&death_observer, &ec]() {death_observer.run(ec);});

    for (unsigned int i = 0; i < child_process_count; i++)
    {
        auto child = core::posix::fork(
                    []() { return core::posix::exit::Status::success; },
        core::posix::StandardStream::stdin | core::posix::StandardStream::stdout);
        death_observer.add(child);
        // A bit ugly but we have to ensure that no signal is lost.
        // And thus, we keep the process object alive.
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
    }

    if (t.joinable())
        t.join();

    EXPECT_FALSE(ec);
    EXPECT_EQ(child_process_count, counter);
}

TEST(StreamRedirect, redirecting_stdin_stdout_stderr_works)
{
    core::posix::ChildProcess child = core::posix::fork(
                []()
                {
                    std::string line;
                    while(true)
                    {
                        std::cin >> line;
                        std::cout << line << std::endl;
                        std::cerr << line << std::endl;
                    }
                    return core::posix::exit::Status::failure;
                },
                core::posix::StandardStream::stdin | core::posix::StandardStream::stdout | core::posix::StandardStream::stderr);

    ASSERT_TRUE(child.pid() > 0);

    const std::string echo_value{"42"};
    child.cin() << echo_value << std::endl;
    std::string line;
    EXPECT_NO_THROW(child.cout() >> line);
    EXPECT_EQ(echo_value, line);
    EXPECT_NO_THROW(child.cerr() >> line);
    EXPECT_EQ(echo_value, line);
    EXPECT_NO_THROW(child.send_signal_or_throw(core::posix::Signal::sig_kill));
    child.wait_for(core::posix::wait::Flags::untraced);
}

TEST(Environment, iterating_the_environment_does_not_throw)
{
    EXPECT_NO_THROW(core::posix::this_process::env::for_each(
                        [](const std::string& key, const std::string& value)
                        {
                            std::cout << key << " -> " << value << std::endl;
                        }););
}

TEST(Environment, specifying_default_value_for_get_returns_correct_result)
{
    const std::string expected_value{"42"};
    EXPECT_EQ(expected_value,
              core::posix::this_process::env::get("totally_non_existant_key_in_env_blubb", expected_value));
}
