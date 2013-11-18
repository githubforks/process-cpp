/*
 * Copyright © 2012-2013 Canonical Ltd.
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
#ifndef TESTING_FORK_AND_RUN_H_
#define TESTING_FORK_AND_RUN_H_

#include <posix/exit.h>
#include <posix/visibility.h>

#include <functional>

namespace testing
{
/**
 * @brief The ForkAndRunResult enum models the different failure modes of fork_and_run.
 */
enum class ForkAndRunResult
{
    empty = 0, ///< Special value indicating no bit being set.
    client_failed = 1 << 0, ///< The client failed.
    service_failed = 1 << 1 ///< The service failed.
};

POSIX_DLL_PUBLIC ForkAndRunResult operator|(ForkAndRunResult lhs, ForkAndRunResult rhs);
POSIX_DLL_PUBLIC ForkAndRunResult operator&(ForkAndRunResult lhs, ForkAndRunResult rhs);

/**
 * @brief Forks two processes for both the service and the client.
 * @throw std::system_error if an error occured during process interaction.
 * @throw std::runtime_error for signalling all other error conditions.
 * @param [in] service The service to be executed in a child process.
 * @param [in] client The client to be executed in a child process.
 * @return ForkAndRunResult indicating if either of service or client failed.
 */
POSIX_DLL_PUBLIC ForkAndRunResult fork_and_run(
        const std::function<posix::exit::Status()>& service,
        const std::function<posix::exit::Status()>& client);
}

#endif // TESTING_FORK_AND_RUN_H_
