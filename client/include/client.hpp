#pragma once

#include <deque>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <string_view>

#include "common.hpp"

#include <boost/asio.hpp>

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
     void Write( std::string_view msg );

     /// @brief Закрытие сокета клиента
     void Close();

     /// @brief Проверка состояния соединения
     /// @return true, если сокет открыт, иначе false
     bool IsConnected() const { return socket_.is_open(); }

private:
     void Read();
     void WriteImpl();

     boost::asio::io_context& io_context_;
     tcp::socket socket_;
     std::string readMessages_;
     common::message_queue writeMessages_;
};

} // namespace client
