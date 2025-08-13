#pragma once

#include <deque>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <string_view>

#include "common.hpp"
#include "database.hpp"

#include <boost/asio.hpp>

namespace server
{

using boost::asio::ip::tcp;

class Session;
class Server;

class ClientConnection : public std::enable_shared_from_this<ClientConnection>
{
public:
     ClientConnection( tcp::socket&& socket, std::shared_ptr<Server>&& server );

     void Start();
     void Deliver( std::string_view msg );
     void Close();

private:
     void Read();
     void Write();
     void RequestSessionId();
     void ReadSessionId();
     void JoinChat( int id );
     void RequestIdentUser();
     void ReadIdentUser();
     void RegisterUser( std::string_view username, std::string_view password );
     void AuthUser( std::string_view username, std::string_view password );

private:
     tcp::socket socket_;

     common::message_queue writeMessages_;
     std::string data_;
     boost::asio::streambuf inputBuffer_;

     std::shared_ptr<Server> server_;
     std::optional<std::shared_ptr<Session>> session_;
     std::string username_;
};

class Session
{
public:
     Session( const int id );

     void Join( const std::shared_ptr<ClientConnection>& clientConn );
     void Leave( const std::shared_ptr<ClientConnection>& clientConn );
     void Deliver( std::string_view msg ) const;
     void Close();

private:
     int id_;
     std::set<std::shared_ptr<ClientConnection>> clientsConn_;

     mutable std::mutex mutex_;
};

class Server : public std::enable_shared_from_this<Server>
{
public:
     Server( boost::asio::io_context& io_context, const tcp::endpoint& endpoint,
             std::string_view connStr, const std::size_t connSize );
     ~Server();

     int CreateSession();
     std::optional<std::shared_ptr<Session>> GetSession( const int id ) const;
     std::shared_ptr<Database> GetDatabase() const;
     void Close();

private:
     void Accept();

     boost::asio::io_context& io_context_;
     tcp::acceptor acceptor_;

     std::map<int, std::shared_ptr<Session>> sessions_;
     std::shared_ptr<Database> db_;

     bool isClose_;
     mutable std::mutex mutex_;
};

} // namespace server
