#include "server.hpp"

namespace server
{

ChatSession::ChatSession( tcp::socket socket, ChatServer& server )
    : socket_( std::move( socket ) ), server_( server )
{
}

void ChatSession::Start()
{
    server_.AddParticipant( shared_from_this() );
    Read();
}

void ChatSession::Deliver( const std::string& msg )
{
    bool write_in_progress = !write_msgs_.empty();
    write_msgs_.push_back( msg );

    if ( !write_in_progress )
    {
        Write();
    }
}

void ChatSession::Read()
{
    auto self( shared_from_this() );

    boost::asio::async_read_until( socket_, boost::asio::dynamic_buffer( buffer_ ), "\n",
        [this, self]( boost::system::error_code ec, std::size_t length ) {
            if ( !ec )
            {
                std::string msg( buffer_.substr( 0, length ) );
                buffer_.erase( 0, length );
                DeliverToAll( msg );
                Read();
            }
            else
            {
                server_.RemoveParticipant( shared_from_this() );
                socket_.close();
            }
        } );
}

void ChatSession::Write()
{
    auto self( shared_from_this() );

    boost::asio::async_write( socket_, boost::asio::buffer( write_msgs_.front() ),
        [this, self]( boost::system::error_code ec, std::size_t length ) {
            if ( !ec )
            {
                write_msgs_.pop_front();
                if ( !write_msgs_.empty() )
                {
                    Write();
                }
            }
            else
            {
                server_.RemoveParticipant( shared_from_this() );
                socket_.close();
            }
        } );
}

void ChatSession::DeliverToAll( const std::string& msg )
{
    for ( auto& participant : server_.participants_ )
    {
        participant->Deliver( msg );
    }
}

ChatServer::ChatServer( boost::asio::io_context& io_context, const tcp::endpoint& endpoint )
    : acceptor_( io_context, endpoint )
{
}

void ChatServer::AddParticipant( std::shared_ptr<ChatSession> participant )
{
    participants_.push_back( participant );
}

void ChatServer::RemoveParticipant( std::shared_ptr<ChatSession> participant )
{
    participants_.erase( std::remove( participants_.begin(), participants_.end(), participant ),
        participants_.end() );
}

void ChatServer::Accept()
{
    acceptor_.async_accept( [this]( boost::system::error_code ec, tcp::socket socket ) {
        if ( !ec )
        {
            auto session = std::make_shared<ChatSession>( std::move( socket ), *this );
            session->Start();
        }
        Accept();
    } );
}

} // namespace server

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
        ChatServer server( io_context, endpoint );
        server.Accept();
        io_context.run();
        std::cout << "break\n";
    }
    catch ( std::exception& e )
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
