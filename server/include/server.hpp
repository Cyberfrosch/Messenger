#pragma once

#ifndef SERVER_HPP
#define SERVER_HPP

#include <boost/asio.hpp>

#include <pqxx/pqxx>

#include <deque>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <utility>

#ifdef DEBUG
#define DEBUG_PRINT( x ) std::cout << x << std::endl
#else
#define DEBUG_PRINT( x )
#endif

namespace server
{

using boost::asio::ip::tcp;
typedef std::deque<std::string> message_queue;

class Session;
class Server;

class ClientConnection : public std::enable_shared_from_this<ClientConnection>
{
public:
     ClientConnection( tcp::socket socket, std::shared_ptr<Server> server );

     void Start();
     void Deliver( const std::string& msg );
     void Close();

private:
     void Read();
     void Write();
     void RequestSessionId();
     void ReadSessionId();
     void JoinChat( int id );

     tcp::socket socket_;
     message_queue writeMessages_;
     std::string data_;
     std::shared_ptr<Server> server_;
     std::shared_ptr<Session> session_;
     boost::asio::streambuf inputBuffer_;
};

class Session
{
public:
     Session( int id );

     void Join( std::shared_ptr<ClientConnection> clientConn );
     void Leave( std::shared_ptr<ClientConnection> clientConn );
     void Deliver( const std::string& msg );
     void Close();

private:
     int id_;
     std::set<std::shared_ptr<ClientConnection>> clientsConn_;
     std::mutex mutex_;
};

class Server : public std::enable_shared_from_this<Server>
{
public:
     Server( boost::asio::io_context& io_context, const tcp::endpoint& endpoint );
     ~Server();

     std::shared_ptr<Session> GetSession( int id );
     int CreateSession();
     void Close();

private:
     void Accept();

     boost::asio::io_context& io_context_;
     tcp::acceptor acceptor_;
     std::map<int, std::shared_ptr<Session>> sessions_;
     bool isClose_;
     std::mutex mutex_;
};

} // namespace server

#endif // SERVER_HPP
