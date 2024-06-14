#include "client.hpp"

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

        std::string host = argv[1];
        std::string port = argv[2];

        tcp::resolver resolver( io_context );
        auto endpoints = resolver.resolve( host, port );

#ifdef DEBUG
        for ( const auto& endpoint : endpoints )
        {
            std::cout << "Resolved address: " << endpoint.endpoint().address().to_string()
                      << std::endl;
        }
#endif // DEBUG

        auto client = std::make_shared<ChatClient>( io_context, *endpoints.begin() );
        client->Start();

        std::thread clientThread( [&io_context]() { io_context.run(); } );
        std::thread inputThread( [&client]() {
            std::string msg;
            while ( std::getline( std::cin, msg ) )
            {
                client->Write( msg + "\n" );
            }
        } );

        clientThread.join();
        client->IsConnected() ? inputThread.join() : client->Close(), inputThread.detach();
    }
    catch ( std::exception& e )
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
