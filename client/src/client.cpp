#include "client.hpp"

#include <format>
#include <print>

namespace client
{

ChatClient::ChatClient( boost::asio::io_context& io_context, const tcp::endpoint& endpoint )
    : io_context_( io_context ), socket_( io_context )
{
     socket_.connect( endpoint );
     std::println( "The connection was successful" );
}

ChatClient::~ChatClient()
{
     Close();
}

void ChatClient::Start()
{
     common::DebugPrint( "Client starts reading the messages from the server" );
     Read();
}

void ChatClient::Write( std::string_view msg )
{
     auto self( shared_from_this() );
     boost::asio::post( socket_.get_executor(), [this, self, msg = std::string( msg )]() {
          bool write_in_progress = !writeMessages_.empty();
          writeMessages_.push_back( std::move( msg ) + "\n" );
          if ( !write_in_progress )
          {
               WriteImpl();
          }
     } );
}

void ChatClient::Read()
{
     auto self( shared_from_this() );
     boost::asio::async_read_until( socket_, boost::asio::dynamic_buffer( readMessages_ ), "\n",
          [this, self]( boost::system::error_code ec, std::size_t length ) {
               if ( !ec )
               {
                    std::string message( readMessages_.substr( 0, length ) );
                    readMessages_.erase( 0, length );
                    std::print( "{}", message );
                    Read();
               }
               else if ( ec != boost::asio::error::eof )
               {
                    std::println( stderr, "Error while reading: {}", ec.message() );
                    socket_.close();
               }
               else
               {
                    std::println( stderr, "Server disconnected" );
                    this->Close();
               }
          } );
}

void ChatClient::WriteImpl()
{
     auto self( shared_from_this() );
     boost::asio::async_write( socket_, boost::asio::buffer( writeMessages_.front() ),
          [this, self]( boost::system::error_code ec, [[maybe_unused]] std::size_t length ) {
               if ( !ec )
               {
                    writeMessages_.pop_front();
                    if ( !writeMessages_.empty() )
                    {
                         WriteImpl();
                    }
               }
               else
               {
                    std::println( stderr, "Error while writing: {}", ec.message() );
                    socket_.close();
               }
          } );
}

void ChatClient::Close()
{
     if ( !socket_.is_open() )
          return;

     socket_.close();
     io_context_.stop();
     common::DebugPrint( "Connection closed\n" );
}

} // namespace client
