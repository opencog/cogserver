
Console Server
==============
The Console Server implements a fast multi-threaded TCP/IPv4 network
Read-Evaluate-Print-Loop (REPL) server. It allows multiple network
clients to get shell-like access ("a console") to the cogserver.

The code here is generic enough that you can build other servers with
it; there's nothing OpenCog or CogServer-specific in here. All you have
to do is to create a derived class from the abstract `class ConsoleSocket`
and supply the pure virtual methods `ConsoleSocket::OnConnection()` and
`ConsoleSocket::OnLine()`.  Then just call `NetworkServer::run()` and
you are good to go.  See the example usage below.

It is weakly assumed that the console is working with text data; it uses
`std::string` as the main API. I suppose it could be tweaked to handle
binary data, but this has not been a requirement.

Why?
----
The idea of a network console server seems to be so generic that you
might think that there would be dozens of such servers to choose from.
No such luck: back around 2008, I was unable to find even one  generic
network server.  It seems that no one has ever felt the need to create
and publish one, as a generic tool. Oh well. So here we are.

Since then ... the world has changed, and now there really are dozens of
such servers.  The strongest contender (that provides a C++ interface;
we need C++ to interact with the AtomSpace) is
[libhv](https://github.com/ithewei/libhv). Most of code here, such as
`ServerSocket.cc` cuould be ripped out and replaced by `libhv`. But no
matter: the current code is stable, it works, its fast, it's debugged.
So no changes are planned.

Operation
---------
This console server listens for and accepts network connections on a
configurable TCPIPv4 port (port 17001 by default). When a connection
is made, the `ConsoleSocket::OnConnection()` pure virtual method is
called.  For each established connection, it forks a new thread.  As
data comes in over the socket, the `ConsoleSocket::OnLine()` pure
virtual method is called.

Closed connections are handled automatically. Connection closure is
handled in such a way that a server can complete pending, unfinished
work, even as the network client disconnected. There's a fair amount
of implementation trickery to allow this to happen.

The multi-threaded handling means that dozens of clients can be handled
without any problems, without any spinloops, dispatching or other dicey
network handler issues.  This code is *fast* -- or rather, its a lot
faster than any other REPL shell server I was able to find.  Also,
bonus: it doesn't jam up, deadlock, crash or fail. It just works. Woot!

The network connections are NOT encrypted (SSL is not used). If you need
encryption, you should route traffic over an ssh tunnel.

The server does NOT provide any user-login management. If you need login
management, that has to be done at the console layer.

The server provides minimal DDOS mitigation by capping the max number of
simultaneously allowed connections. This number is configurable, and
defaults to ten; additional connections are forced to wait. The `status`
and `top` commands list the connection status.

Example Usage
-------------
Here is a short example. It provides anidea of how simple this is to
use.
```
class MyServerConsole : public ConsoleSocket
{
protected:
	void OnConnection(void)
	{
		Send("my-server-prompt>");
	}
	void OnLine(const std::string& cmd)
	{
		std::string reply = "Recieved this command: " + cmd + "\n";
		Send(reply);
		Send("my-server-prompt>");
	}
};

main()
{
	// Create a server listening on port 4242
	NetworkServer* ns = new NetworkServer(4242);

	// Create a new handler for each connection.
	// The delete is handled automatically.
	auto make_console = [](void)->ConsoleSocket*
        { return new MyServerConsole(); };

	// Process incoming commands.
	ns->run(make_console);
}
```
