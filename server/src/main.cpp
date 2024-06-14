#include "server.hpp"

namespace
{

void SignalHandler( int signal )
{
    switch ( signal )
    {
        case SIGINT:
        {
            DEBUG_PRINT( "SIGINT received" );
            break;
        }
        case SIGQUIT:
        {
            DEBUG_PRINT( "SIGQUIT received" );
            break;
        }
        default:
        {
            break;
        }
    }

    std::cout << "Press Enter to stop the server." << std::endl;
}

} // anonymous namespace

int main( int argc, char* argv[] )
{
    try
    {
        if ( argc != 2 )
        {
            std::cerr << "Usage: chat_server <port>" << std::endl;
            return 1;
        }

        using namespace server;

        boost::asio::io_context io_context;
        tcp::endpoint endpoint( tcp::v4(), std::atoi( argv[1] ) );
        auto server = std::make_shared<ChatServer>( io_context, endpoint );

        std::thread serverThread( [&io_context]() { io_context.run(); } );

        if ( std::signal( SIGINT, SignalHandler ) == SIG_ERR ||
             std::signal( SIGQUIT, SignalHandler ) == SIG_ERR )
        {
            std::cerr << "Cannot set signal handler" << std::endl;
        }

        std::cout << "Press Enter to stop the server." << std::endl;
        std::cin.get();
        server->Close();

        serverThread.join();
    }
    catch ( std::exception& e )
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
