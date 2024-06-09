#pragma once

#ifndef _CLIENT_HPP_
#define _CLIENT_HPP_

#include <boost/asio.hpp>

#include <deque>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

namespace client
{

using boost::asio::ip::tcp;

/// @brief Класс клиента чата
class ChatClient : public std::enable_shared_from_this<ChatClient>
{
public:
    /// @brief Конструктор клиента чата
    /// @param io_context Контекст ввода-вывода для асинхронной работы
    /// @param endpoint Конечная точка сервера (IP-адрес и порт)
    ChatClient( boost::asio::io_context& io_context, const tcp::endpoint& endpoint );
    ~ChatClient();

    /// @brief Начинает общение с сервером
    void Start();

    /// @brief Отправляет сообщение серверу
    /// @param msg Сообщение для отправки
    void Write( const std::string& msg );

    /// @brief Закрытие сокета клиента
    void Close();

    /// @brief Проверка состояния соединения
    /// @return true, если сокет открыт, иначе false
    bool IsConnected() const;

private:
    void Read();
    void WriteImpl();

    boost::asio::io_context& io_context_;
    tcp::socket socket_;
    std::string readMsg_;
    std::deque<std::string> writeMsgs_;
};

} // namespace client

#endif // _CLIENT_HPP_
