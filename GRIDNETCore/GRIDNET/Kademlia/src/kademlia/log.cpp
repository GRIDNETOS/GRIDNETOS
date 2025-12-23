// Copyright (c) 2014, David Keller
// All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of the University of California, Berkeley nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY DAVID KELLER AND CONTRIBUTORS ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "kademlia/log.hpp"

#include <iostream>
#include <set>
#include <NetworkManager.h>
#include "BlockchainManager.h"
#include "Tools.h"

namespace kademlia {
namespace detail {

namespace {

using enabled_modules_log_type = std::set< std::string >;

enabled_modules_log_type &
get_enabled_modules
    ( void )
{
    static enabled_modules_log_type enabled_modules_;
    return enabled_modules_;
}

} // anonymous namespace

std::string kademliaSubsystemToString(eKademliaSubsystem::eKademliaSubsystem subsystem)
{
    switch (subsystem)
    {
    case eKademliaSubsystem::engine:
        return "Engine";
        break;
    case eKademliaSubsystem::discover_neighbors_task:
        return "Discovery";
        break;
    case eKademliaSubsystem::routing_table:
        return "Routing#";
        break;
    case eKademliaSubsystem::find_value_task:
        return "#Retrieval";
        break;
    case eKademliaSubsystem::store_value_task:
        return "#Storage";
        break;
    case eKademliaSubsystem::lookup_task:
        return "Lookups";
        break;
    case eKademliaSubsystem::notify_peer_task:
        return "Notifications";
        break;
    case eKademliaSubsystem::response_router:
        return "Routing<";
        break;
    default:
        return "unknown";
        break;
    }
    return "unknown";
}
std::ostream &
get_debug_log
    (const std::string& txt,eKademliaSubsystem::eKademliaSubsystem subsystem
    , void const * thiz, eColor::eColor color, eLogEntryType::eLogEntryType logEntryType)
{
    std::shared_ptr<CBlockchainManager> bm = CBlockchainManager::getInstance(eBlockchainMode::TestNet);
    std::shared_ptr<CTools> tools = bm->getTools();
    std::shared_ptr<CNetworkManager> nm = bm->getNetworkManager();

    std::lock_guard<std::mutex> lock(sLogGuardian);
    if (color != eColor::none)
    {
        const_cast<std::string&>(txt) = tools->getColoredString(txt, color);
    }

    //Notice: in Network Test Mode - logs are enforced below (last parameter):
    if (nm->getIsNetworkTestMode() && nm->canEventLogAboutIP("any"))
    {
        tools->logEvent(tools->getColoredString("[Kademlia: " + kademliaSubsystemToString(subsystem) + "]: ", eColor::lightCyan) + txt, eLogEntryCategory::network, color, logEntryType == eLogEntryType::failure ? 3 : 0, logEntryType, nm->getIsNetworkTestMode());
    }
    sSS.str(std::string());//reset
    return sSS;
      
}

/**
 *
 */
void
enable_log_for
    ( std::string const& module )
{ get_enabled_modules().insert( module ); }

/**
 *
 */
void
disable_log_for
    ( std::string const& module )
{ get_enabled_modules().erase( module ); }

/**
 *
 */
bool
is_log_enabled
    ( std::string const& module )
{
    return get_enabled_modules().count( "*" ) > 0
            || get_enabled_modules().count( module ) > 0;
}

} // namespace detail
} // namespace kademlia

