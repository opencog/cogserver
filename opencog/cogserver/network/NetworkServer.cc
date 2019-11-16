/*
 * opencog/cogserver/server/NetworkServer.cc
 *
 * Copyright (C) 2002-2007 Novamente LLC
 * Copyright (C) 2008 by OpenCog Foundation
 * Written by Andre Senna <senna@vettalabs.com>
 *            Gustavo Gama <gama@vettalabs.com>
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <opencog/util/Logger.h>
#include <opencog/cogserver/network/ConsoleSocket.h>

#include "NetworkServer.h"

using namespace opencog;

NetworkServer::NetworkServer(unsigned short port) :
    _running(false),
    _port(port),
    _acceptor(_io_service,
        boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
{
    logger().debug("[NetworkServer] constructor");
}

NetworkServer::~NetworkServer()
{
    logger().debug("[NetworkServer] enter destructor");

    stop();

    logger().debug("[NetworkServer] all threads joined, exit destructor");
}

void NetworkServer::stop()
{
    if (not _running) return;
    _running = false;

    boost::system::error_code ec;
    _acceptor.cancel(ec);
    _io_service.stop();

    // Booost::asio hangs, despite the above.  Brute-force it to
    // get it's head out of it's butt, and do the right thing.
    pthread_cancel(_listener_thread->native_handle());

    _listener_thread->join();
    delete _listener_thread;
    _listener_thread = nullptr;
}

void NetworkServer::listen(void)
{
    printf("Listening on port %d\n", _port);
    while (_running)
    {
        // The call to _acceptor.accept() will block this thread until
        // a network connection is made. Thus, we defer the creation
        // of the connection handler thread until after accept()
        // returns.  However, the boost design violates RAII principles,
        // so instead, what we do is to hand off the socket created here,
        // to the ServerSocket class, which will delete it, when the
        // connection socket closes (just before the connection handler
        // thread exits).  That is why there is no delete of the *ss
        // below, and that is why there is the weird self-delete at the
        // end of ServerSocket::handle_connection().
        boost::asio::ip::tcp::socket* sock = new boost::asio::ip::tcp::socket(_io_service);
        _acceptor.accept(*sock);

        // The total number of concurrently open sockets is managed by
        // keeping a count in ConsoleSocket, and blocking when there are
        // too many.
        ConsoleSocket* ss = _getConsole();
        ss->set_connection(sock);
        std::thread(&ConsoleSocket::handle_connection, ss).detach();
    }
}

void NetworkServer::run(ConsoleSocket* (*handler)(void))
{
    if (_running) return;
    _running = true;
    _getConsole = handler;

    try {
        _io_service.run();
    } catch (boost::system::system_error& e) {
        logger().error("Error in boost::asio io_service::run() => %s", e.what());
    }

    _listener_thread = new std::thread(&NetworkServer::listen, this);
}
