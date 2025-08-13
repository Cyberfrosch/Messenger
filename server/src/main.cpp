#include "server.hpp"

#include <cstdlib>
#include <stacktrace>
#include <thread>

namespace
{

void SignalHandler( int signal )
{
     switch ( signal )
     {
          case SIGINT:
          {
               common::DebugPrint( "SIGINT received\n" );
               break;
          }
          case SIGQUIT:
          {
               common::DebugPrint( "SIGQUIT received\n" );
               break;
          }
          case SIGTERM:
          {
               common::DebugPrint( "SIGTERM received\n" );
               break;
          }
          default:
          {
               break;
          }
     }

     std::println( "Press <Enter> to stop the server" );
}

} // anonymous namespace

int main( int argc, char* argv[] )
{
     try
     {
          if ( std::signal( SIGINT, SignalHandler ) == SIG_ERR ||
               std::signal( SIGQUIT, SignalHandler ) == SIG_ERR ||
               std::signal( SIGTERM, SignalHandler ) == SIG_ERR )
          {
               std::cerr << "Cannot set signal handler" << std::endl;
               return EXIT_FAILURE;
          }

          if ( argc != 2 )
          {
               std::cerr << "Usage: chat_server <port>" << std::endl;
               return EXIT_FAILURE;
          }

          using namespace server;

          boost::asio::io_context io_context;
          tcp::endpoint endpoint( tcp::v6(), std::atoi( argv[1] ) );

          const char* envConnStr = std::getenv("DB_CONNECTION_STRING");
          std::string_view connStr = envConnStr ? envConnStr :
               "dbname=messenger_db user=messenger password=123 host=localhost port=5432";
          auto server = std::make_shared<Server>( io_context, endpoint, connStr, 10 );

          std::jthread serverThread( [&io_context]() { io_context.run(); } );

          std::println( "Press <Enter> to stop the server" );
          std::cin.get();
          server->Close();
     }
     catch ( std::exception& e )
     {
          std::println( stderr, "Exception: {}", e.what() );
#ifdef __cpp_lib_stacktrace
          std::println( stderr, "Stack trace:\n{}", std::stacktrace::current() );
#endif
          return EXIT_FAILURE;
     }

     return EXIT_SUCCESS;
}
