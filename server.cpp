// server.cpp

#include "server.hpp"

#ifdef DEBUG
#define DEBUG_PRINT( x ) std::cout << x << std::endl
#else
#define DEBUG_PRINT( x )
#endif

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
    bool writeInProgress = !writeMsgs_.empty();
    writeMsgs_.push_back( msg );

    if ( !writeInProgress )
    {
        Write();
    }

    DEBUG_PRINT( "Message delivered: " << msg );
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
                DEBUG_PRINT( "Received message: " << msg );
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

    boost::asio::async_write( socket_, boost::asio::buffer( writeMsgs_.front() ),
        [this, self]( boost::system::error_code ec, std::size_t length ) {
            if ( !ec )
            {
                writeMsgs_.pop_front();
                if ( !writeMsgs_.empty() )
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

    DEBUG_PRINT( "Message sent: " << writeMsgs_.front() );
}

void ChatSession::DeliverToAll( const std::string& msg )
{
    for ( auto& participant : server_.participants_ )
    {
        participant->Deliver( msg );
    }
}

void ChatSession::Close()
{
    socket_.close();
}

ChatServer::ChatServer( boost::asio::io_context& io_context, const tcp::endpoint& endpoint )
    : io_context_( io_context ), acceptor_( io_context, endpoint ), isClose( false )
{
    std::cout << "Server has been started" << std::endl;
    Accept();
}

ChatServer::~ChatServer()
{
    std::cout << "Server has been closed" << std::endl;
    Close();
}

void ChatServer::AddParticipant( std::shared_ptr<ChatSession> participant )
{
    std::lock_guard<std::mutex> lock( mutex_ );

    participants_.push_back( participant );
    DEBUG_PRINT( "Participant added, current size: " << participants_.size() );
}

void ChatServer::RemoveParticipant( std::shared_ptr<ChatSession> participant )
{
    std::lock_guard<std::mutex> lock( mutex_ );

    participants_.erase( std::remove( participants_.begin(), participants_.end(), participant ),
        participants_.end() );
    DEBUG_PRINT( "Participant removed, current size: " << participants_.size() );
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

void ChatServer::Close()
{
    std::lock_guard<std::mutex> lock( mutex_ );

    if ( isClose )
    {
        return;
    }

    DEBUG_PRINT( "Participants size before close: " << participants_.size() );
    for ( auto& participant : participants_ )
    {
        participant->Close();
    }
    participants_.clear();
    DEBUG_PRINT( "Participants size after close: " << participants_.size() );

    acceptor_.close();
    io_context_.stop();
    isClose = true;
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
