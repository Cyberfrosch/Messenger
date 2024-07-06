#include "client.hpp"

namespace client
{

ChatClient::ChatClient( boost::asio::io_context& io_context, const tcp::endpoint& endpoint )
    : io_context_( io_context ), socket_( io_context )
{
     socket_.connect( endpoint );
     std::cout << "The connection was successful" << std::endl;
}

ChatClient::~ChatClient()
{
     Close();
}

void ChatClient::Start()
{
     DEBUG_PRINT( "Client starts reading the messages from the server" << std::endl );
     Read();
}

void ChatClient::Write( const std::string& msg )
{
     auto self( shared_from_this() );
     boost::asio::post( socket_.get_executor(), [this, self, msg]() {
          bool write_in_progress = !writeMessages_.empty();
          writeMessages_.push_back( msg + "\n" );
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
                    // TODO: Не отправлять сообщение создателю этого сообщения
                    std::cout << message;
                    Read();
               }
               else if ( ec != boost::asio::error::eof )
               {
                    std::cerr << "Error in reading: " << ec.message() << std::endl;
                    socket_.close();
               }
               else
               {
                    std::cerr << "Server disconnected." << std::endl;
                    this->Close();
               }
          } );
}

void ChatClient::WriteImpl()
{
     auto self( shared_from_this() );
     boost::asio::async_write( socket_, boost::asio::buffer( writeMessages_.front() ),
          [this, self]( boost::system::error_code ec, std::size_t length ) {
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
                    std::cerr << "Error in writing: " << ec.message() << std::endl;
                    socket_.close();
               }
          } );
}

void ChatClient::Close()
{
     if ( !socket_.is_open() )
     {
          return;
     }
     socket_.close();
     io_context_.stop();
     DEBUG_PRINT( "Connection close" << std::endl );
}

bool ChatClient::IsConnected() const
{
     return socket_.is_open();
}

} // namespace client
