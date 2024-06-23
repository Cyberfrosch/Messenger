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
            return EXIT_FAILURE;
        }

        using namespace server;

        boost::asio::io_context io_context;
        tcp::endpoint endpoint( tcp::v6(), std::atoi( argv[1] ) );

        const std::string connectionString = "dbname=messenger_db user=messenger "
                                             "password=123 hostaddr=127.0.0.1 port=5432";

        pqxx::connection conn( connectionString );
        if ( conn.is_open() )
        {
            std::cout << "Opened database successfully: " << conn.dbname() << std::endl;
        }
        else
        {
            std::cerr << "Can't open database\n";
            return EXIT_FAILURE;
        }

        conn.prepare(
            "authenticate_user", "SELECT * FROM users WHERE username = $1 AND password = $2" );
        conn.prepare( "register_user", "INSERT INTO users (username, password) VALUES ($1, $2)" );

        auto server = std::make_shared<ChatServer>( io_context, endpoint, conn );

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

    return EXIT_SUCCESS;
}
