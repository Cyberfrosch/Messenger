#pragma once

#ifndef _SERVER_HPP_
#define _SERVER_HPP_

#include <boost/asio.hpp>

#include <algorithm>
#include <deque>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace server
{

using boost::asio::ip::tcp;

class ChatServer;

/// @brief Класс сессии чата
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

private:
    /// @brief Считывание отправляемых в чат данных
    void Read();
    /// @brief Запись полученных данных
    void Write();
    /// @brief Рассылка сообщения всем участникам чата
    /// @param msg Отправляемое сообщение
    void DeliverToAll( const std::string& msg );

    tcp::socket socket_;
    std::string buffer_;
    std::deque<std::string> write_msgs_;
    ChatServer& server_;
};

/// @brief Класс серверного чата
class ChatServer
{
public:
    /// @brief Конструктор серверного чата
    /// @param io_context Контекст ввода-вывода для асинхронной работы
    /// @param endpoint Конечная точка сервера (IP-адрес и порт)
    ChatServer( boost::asio::io_context& io_context, const tcp::endpoint& endpoint );

    /// @brief Добавление участника чата
    /// @param participant
    void AddParticipant( std::shared_ptr<ChatSession> participant );

    /// @brief Удаление участника чата
    /// @param participant Участник чата
    void RemoveParticipant( std::shared_ptr<ChatSession> participant );

    /// @brief Запуск прослушивания входящих соединений
    void Accept();

private:
    friend class ChatSession;

    std::vector<std::shared_ptr<ChatSession>> participants_;
    tcp::acceptor acceptor_;
};

} // namespace server

#endif // _SERVER_HPP_
