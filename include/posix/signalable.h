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

#ifndef POSIX_SIGNALABLE_H_
#define POSIX_SIGNALABLE_H_

#include <posix/signal.h>
#include <posix/visibility.h>

#include <memory>
#include <system_error>

namespace posix
{
/**
 * @brief The Signalable class abstracts the ability of an entity to be delivered a posix signal.
 */
class POSIX_DLL_PUBLIC Signalable
{
public:
    /**
     * @brief Sends a signal to this signalable object.
     * @throws std::system_error in case of problems.
     * @param [in] signal The signal to be sent to the process.
     */
    virtual void send_signal_or_throw(Signal signal);

    /**
     * @brief Sends a signal to this signalable object.
     * @param [in] signal The signal to be sent to the process.
     * @param [out] e Set to contain an error if an issue arises.
     */
    virtual void send_signal(Signal signal, std::error_code& e) noexcept(true);

protected:
    POSIX_DLL_LOCAL explicit Signalable(pid_t pid);

private:
    struct POSIX_DLL_LOCAL Private;
    std::shared_ptr<Private> d;
};
}

#endif // POSIX_SIGNALABLE_H_