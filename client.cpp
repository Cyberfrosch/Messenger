#include "client.hpp"

#ifdef DEBUG
#define DEBUG_PRINT( x ) std::cout << x << std::endl
#else
#define DEBUG_PRINT( x )
#endif

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
    DEBUG_PRINT( "Client starts reading the messages from the server" );
    Read();
}

void ChatClient::Write( const std::string& msg )
{
    auto self( shared_from_this() );
    boost::asio::post( socket_.get_executor(), [this, self, msg]() {
        bool write_in_progress = !writeMsgs_.empty();
        writeMsgs_.push_back( msg );
        if ( !write_in_progress )
        {
            WriteImpl();
        }
    } );
}

void ChatClient::Read()
{
    auto self( shared_from_this() );
    boost::asio::async_read_until( socket_, boost::asio::dynamic_buffer( readMsg_ ), "\n",
        [this, self]( boost::system::error_code ec, std::size_t length ) {
            if ( !ec )
            {
                std::cout << "Server: " << std::string( readMsg_.substr( 0, length ) );
                readMsg_.erase( 0, length );
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
    boost::asio::async_write( socket_, boost::asio::buffer( writeMsgs_.front() ),
        [this, self]( boost::system::error_code ec, std::size_t length ) {
            if ( !ec )
            {
                writeMsgs_.pop_front();
                if ( !writeMsgs_.empty() )
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
    DEBUG_PRINT( "Connection close" );
}

bool ChatClient::IsConnected() const
{
    return socket_.is_open();
}

} // namespace client

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
