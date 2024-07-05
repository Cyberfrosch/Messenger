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
#include <queue>
#include <set>
#include <utility>

#ifdef DEBUG
#define DEBUG_PRINT( x ) std::cout << x
#else
#define DEBUG_PRINT( x )
#endif

namespace server
{

using boost::asio::ip::tcp;
typedef std::deque<std::string> message_queue;

class Session;
class Server;

class Database
{
public:
     Database( const std::string& connStr, std::size_t poolSize );

     pqxx::result ExecQuery( const std::string& query );
     template <typename... Args>
     pqxx::result ExecPreparedQuery( const std::string& stmt, Args&&... args );

     void ExecUpdate( const std::string& query );
     template <typename... Args>
     void ExecPreparedUpdate( const std::string& stmt, Args&&... args );

     void PrepareStatements( std::shared_ptr<pqxx::connection> conn );

private:
     std::shared_ptr<pqxx::connection> GetConnection();
     void FreeConnection( std::shared_ptr<pqxx::connection> conn );

     std::string connStr_;
     std::queue<std::shared_ptr<pqxx::connection>> pool_;

     std::mutex mutex_;
     std::condition_variable condition_;
};

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
