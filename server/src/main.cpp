#include "server.hpp"

namespace
{

void SignalHandler( int signal )
{
     switch ( signal )
     {
          case SIGINT:
          {
               DEBUG_PRINT( "SIGINT received" << std::endl );
               break;
          }
          case SIGQUIT:
          {
               DEBUG_PRINT( "SIGQUIT received" << std::endl );
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
          if ( std::signal( SIGINT, SignalHandler ) == SIG_ERR || std::signal( SIGQUIT, SignalHandler ) == SIG_ERR )
          {
               std::cerr << "Cannot set signal handler" << std::endl;
          }

          if ( argc != 2 )
          {
               std::cerr << "Usage: chat_server <port>" << std::endl;
               return EXIT_FAILURE;
          }

          using namespace server;

          boost::asio::io_context io_context;
          tcp::endpoint endpoint( tcp::v6(), std::atoi( argv[1] ) );

          const std::string connStr = "dbname=messenger_db user=messenger "
                                      "password=123 host=localhost port=5432";
          auto server = std::make_shared<Server>( io_context, endpoint, connStr, 10 );

          std::thread serverThread( [&io_context]() { io_context.run(); } );

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
