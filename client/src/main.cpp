#include "client.hpp"

#include <cstdlib>
#include <print>
#include <stacktrace>

int main( int argc, char* argv[] )
{
     try
     {
          if ( argc != 3 )
          {
               std::println( stderr, "Usage: chat_client <host> <port>" );
               return EXIT_FAILURE;
          }

          using namespace client;

          boost::asio::io_context io_context;

          std::string host = argv[1];
          std::string port = argv[2];

          tcp::resolver resolver( io_context );
          auto endpoints = resolver.resolve( host, port );

          if constexpr ( common::isDebug )
          {
               for ( const auto& endpoint : endpoints )
               {
                    std::println( "Resolved address: {}", endpoint.endpoint().address().to_string() );
               }
          }

          auto client = std::make_shared<ChatClient>( io_context, *endpoints.begin() );
          client->Start();

          std::jthread clientThread( [&io_context]() {
               io_context.run();
          } );

          std::jthread inputThread( [client]() {
               std::string msg;
               while ( std::getline( std::cin, msg ) )
               {
                    if ( !client->IsConnected() )
                    {
                         std::println( stderr, "Connection lost. Exiting input thread" );
                         return;
                    }
                    client->Write( msg );
               }
          } );
     }
     catch ( std::exception& e )
     {
          std::println( stderr, "Exception: {}\nStack trace:\n{}", e.what(), std::stacktrace::current() );
#ifdef __cpp_lib_stacktrace
          std::println( stderr, "Stack trace:\n{}", std::stacktrace::current() );
#endif
          return EXIT_FAILURE;
     }

     return EXIT_SUCCESS;
}
