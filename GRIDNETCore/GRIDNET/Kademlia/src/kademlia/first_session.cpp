// Copyright (c) 2013-2014, David Keller
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

#include <kademlia/first_session.hpp>

#include "session_impl.hpp"
#include "EEndPoint.h"
class CNetworkManager;
class CTools;
namespace kademlia {

/**
 *
 */
struct first_session::impl final
        : detail::session_impl
{
    /**
     *
     */
    impl
        (::std::shared_ptr<CNetworkManager> networkManager, endpoint const& listen_on_ipv4
        , endpoint const& listen_on_ipv6 )
            : session_impl{ networkManager, listen_on_ipv4
                          , listen_on_ipv6 }
    { }
};

first_session::first_session
    (::std::shared_ptr<CNetworkManager>networkManager,
        endpoint const& listen_on_ipv4
    , endpoint const& listen_on_ipv6 )
        : impl_{ new impl{ networkManager, listen_on_ipv4, listen_on_ipv6 } }
{ }

bool first_session::processUnsolicitedDatagram(std::vector<uint8_t> bytes, const CEndPoint & endpoint)
{
   return impl_->processUnsolicitedDatagram(bytes, endpoint);
 
}

first_session::~first_session
    ( void )
{ }

std::vector<std::shared_ptr<CEndPoint>> first_session::getPeers()
{
    assertGN(impl_ != NULL);
    std::vector<std::shared_ptr<CEndPoint>> toRett;
    std::vector<kademlia::detail::ip_endpoint> ipEps = impl_->getPeers();
    for (int i = 0; i < ipEps.size(); i++)
    {
        std::shared_ptr<CEndPoint> c = std::make_shared<CEndPoint>(CTools::getInstance()->stringToBytes(ipEps[i].address_.to_string()), ipEps[i].address_.is_v4() ? eEndpointType::IPv4 :
            eEndpointType::IPv6, nullptr, ipEps[i].port_);
        c->setPubKey(ipEps[i].pubKey);

        toRett.push_back(c);
    }
    return toRett;
}

std::error_code
first_session::run
    ( void )
{ return impl_->run(); }

void
first_session::abort
        ( void )
{ impl_->abort(); }

} // namespace kademlia

