#pragma once

#ifndef SERVER_HPP
#define SERVER_HPP

#include <boost/asio.hpp>

#include <deque>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <set>

#include "common.hpp"
#include "database.hpp"
namespace server
{

using boost::asio::ip::tcp;

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
     void RequestIdentUser();
     void ReadIdentUser();
     void RegisterUser( const std::string& username, const std::string& password );
     void AuthUser( const std::string& username, const std::string& password );

     tcp::socket socket_;

     message_queue writeMessages_;
     std::string data_;
     boost::asio::streambuf inputBuffer_;

     std::shared_ptr<Server> server_;
     std::shared_ptr<Session> session_;
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
     Server( boost::asio::io_context& io_context, const tcp::endpoint& endpoint, const std::string& connStr,
          std::size_t connSize );
     ~Server();

     int CreateSession();
     std::shared_ptr<Session> GetSession( int id );
     std::shared_ptr<Database> GetDatabase() const;
     void Close();

private:
     void Accept();

     boost::asio::io_context& io_context_;
     tcp::acceptor acceptor_;

     std::map<int, std::shared_ptr<Session>> sessions_;
     std::shared_ptr<Database> db_;

     bool isClose_;
     std::mutex mutex_;
};

} // namespace server

#endif // SERVER_HPP
