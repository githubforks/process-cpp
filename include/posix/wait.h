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

#ifndef POSIX_WAIT_H_
#define POSIX_WAIT_H_

#include <posix/exit.h>
#include <posix/signal.h>

#include <bitset>

#include <sys/wait.h>

namespace posix
{
namespace wait
{
    enum Flag
    {
        continued = WCONTINUED,
        untraced = WUNTRACED,
        no_hang = WNOHANG
    };
    typedef std::uint32_t Flags;

    /**
     * @brief The Result struct encapsulates the result of waiting for a process state change.
     */
    struct Result
    {
        /**
         * @brief The status of the process/wait operation.
         */
        enum class Status
        {
            undefined, ///< Marks an undefined state.
            no_state_change, ///< No state change occured.
            exited, ///< The process exited normally.
            signaled, ///< The process was signalled and terminated.
            stopped, ///< The process was signalled and stopped.
            continued ///< The process resumed operation.
        } status = Status::undefined;

        /**
         * @brief Union of result-specific details.
         */
        union
        {
            /**
             * Contains the exit status of the process if status == Status::exited.
             */
            struct
            {
                exit::Status status; ///< Exit status of the process.
            } if_exited;

            /**
             * Contains the signal that caused the process to terminate if status == Status::signaled.
             */
            struct
            {
                Signal signal; ///< Signal that caused the process to terminate.
                bool core_dumped; ///< true if the process termination resulted in a core dump.
            } if_signaled;

            /**
             * Contains the signal that caused the process to terminate if status == Status::stopped.
             */
            struct
            {
                Signal signal; ///< Signal that caused the process to terminate.
            } if_stopped;
        } detail;
    };
}
}

#endif // POSIX_WAIT_H_
