#pragma once

#ifndef _SERVER_HPP_
#define _SERVER_HPP_

#include <boost/asio.hpp>

#include <deque>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using boost::asio::ip::tcp;

class ChatSession : public std::enable_shared_from_this<ChatSession>
{
public:
    ChatSession( tcp::socket socket ) : socket_( std::move( socket ) ) {}
    void Start();
    void Deliever( const std::string& msg );

    static std::vector<std::shared_ptr<ChatSession>> participants_;

private:
    void Read();
    void Write();
    void DelieverToAll( const std::string& msg );

    void TestFunc( const std::string& test ) {}

    tcp::socket socket_;
    std::string buffer_;
    std::deque<std::string> write_msgs_;
};

class ChatServer
{
public:
    ChatServer( boost::asio::io_context& io_context, const tcp::endpoint& endpoint )
        : acceptor_( io_context, endpoint )
    {
        accept();
    }

private:
    void accept();

    tcp::acceptor acceptor_;
};

#endif // _SERVER_HPP_
