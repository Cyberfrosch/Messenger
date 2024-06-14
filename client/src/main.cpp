#include "client.hpp"

namespace
{

/// @brief Функция ввода данных для отправки на сервер
/// @param client Указатель на реализацию клиента
void WaitForInput( std::shared_ptr<client::ChatClient> client )
{
    std::string msg;
    while ( std::getline( std::cin, msg ) )
    {
        client->Write( msg + "\n" );
    }
}

} // anonymous namespace

int main( int argc, char* argv[] )
{
    try
    {
        if ( argc != 3 )
        {
            std::cerr << "Usage: chat_client <host> <port>" << std::endl;
            return 1;
        }

        using namespace client;

        boost::asio::io_context io_context;
        tcp::resolver resolver( io_context );
        auto endpoints = resolver.resolve( argv[1], argv[2] );
        auto client = std::make_shared<client::ChatClient>( io_context, *endpoints.begin() );
        client->Start();

        std::thread clientThread( [&io_context]() { io_context.run(); } );
        std::thread inputThread( WaitForInput, client );

        clientThread.join();
        client->IsConnected() ? inputThread.join() : client->Close(), inputThread.detach();
    }
    catch ( std::exception& e )
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
