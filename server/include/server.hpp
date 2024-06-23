#pragma once

#ifndef _SERVER_HPP_
#define _SERVER_HPP_

#include <boost/asio.hpp>

#include <pqxx/pqxx>

#include <algorithm>
#include <csignal>
#include <deque>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#ifdef DEBUG
#define DEBUG_PRINT( x ) std::cout << x << std::endl
#else
#define DEBUG_PRINT( x )
#endif

namespace server
{

using boost::asio::ip::tcp;

class ChatServer;

class AuthSession : public std::enable_shared_from_this<AuthSession>
{
public:
    AuthSession( tcp::socket socket, ChatServer& server, pqxx::connection& conn );

    void Start();

private:
    void Read();
    void HandleAuthMessage( const std::string& msg );
    void AsyncLogin( const std::string& username, const std::string& password );
    void AsyncRegister( const std::string& username, const std::string& password );

    tcp::socket socket_;
    std::string buffer_;
    ChatServer& server_;
    pqxx::connection& conn_;
};

/// @brief Класс серверной сессии чата
class ChatSession : public std::enable_shared_from_this<ChatSession>
{
public:
    /// @brief Конструктор чата
    /// @param socket Сокет для подключения
    /// @param server Сервер, на котором запущена сессия чата
    ChatSession( tcp::socket socket, ChatServer& server );

    /// @brief Запуск сессии чата
    void Start();

    /// @brief Отправка сообщения клиенту
    /// @param msg Отправляемое сообщение
    void Deliver( const std::string& msg );

    /// @brief Закрытие серверной сессии чата
    void Close();

private:
    void Read();
    void Write();
    void DeliverToAll( const std::string& msg );

    tcp::socket socket_;
    std::string buffer_;
    std::deque<std::string> writeMsgs_;
    ChatServer& server_;
};

/// @brief Класс серверного чата
class ChatServer
{
public:
    /// @brief Конструктор серверного чата
    /// @param io_context Контекст ввода-вывода для асинхронной работы
    /// @param endpoint Конечная точка сервера (IP-адрес и порт)
    ChatServer( boost::asio::io_context& io_context, const tcp::endpoint& endpoint,
        pqxx::connection& conn );
    ~ChatServer();

    /// @brief Добавление участника чата
    /// @param participant
    void AddParticipant( std::shared_ptr<ChatSession> participant );

    /// @brief Удаление участника чата
    /// @param participant Участник чата
    void RemoveParticipant( std::shared_ptr<ChatSession> participant );

    /// @brief Закрытие сокета сервера
    void Close();

private:
    friend class ChatSession;

    void Accept();

    boost::asio::io_context& io_context_;
    tcp::acceptor acceptor_;
    std::vector<std::shared_ptr<ChatSession>> participants_;
    std::mutex mutex_;
    bool isClose = false;
    pqxx::connection& conn_;
};

} // namespace server

#endif // _SERVER_HPP_
