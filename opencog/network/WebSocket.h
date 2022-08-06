/*
 * opencog/network/WebSocket.h
 *
 * Copyright (C) 2022 Linas Vepstas
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#ifndef _OPENCOG_WEB_SOCKET_H
#define _OPENCOG_WEB_SOCKET_H

#include <string>

#include <opencog/network/ServerSocket.h>

namespace opencog
{
/** \addtogroup grp_server
 *  @{
 */

/**
 * This class implements a ServerSocket that handles the 
 * web-server-style RESTful connections.
 */
class WebSocket : public ServerSocket
{
private:

protected:

    /** Status printing */
    virtual std::string connection_header(void);
    virtual std::string connection_stats(void);
public:
    /**
     * Ctor. Defines the socket's mime-type as 'text/plain' and then
     * configures the Socket to use line protocol.
     */
    WebSocket(void);
    ~WebSocket();

    virtual void handle_connection(void);

}; // class

/** @}*/
}  // namespace

#endif // _OPENCOG_WEB_SOCKET_H
