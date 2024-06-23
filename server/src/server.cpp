#include "server.hpp"

namespace server
{

AuthSession::AuthSession( tcp::socket socket, ChatServer& server, pqxx::connection& conn )
    : socket_( std::move( socket ) ), server_( server ), conn_( conn )
{
}

void AuthSession::Start()
{
    Read();
}

void AuthSession::Read()
{
    auto self( shared_from_this() );

    boost::asio::async_read_until( socket_, boost::asio::dynamic_buffer( buffer_ ), "\n",
        [this, self]( boost::system::error_code ec, std::size_t length ) {
            if ( !ec )
            {
                std::string msg( buffer_.substr( 0, length ) );
                buffer_.erase( 0, length );
                HandleAuthMessage( msg );
            }
            else
            {
                socket_.close();
                std::cout << "pizdec socket zakrilsa" << std::endl;
            }
        } );
}

void AuthSession::HandleAuthMessage( const std::string& msg ) // register test1 test2
{
    std::istringstream iss( msg );
    std::string command;
    iss >> command;
    std::cout << "command: " << command << std::endl;

    if ( command == "register" )
    {
        std::string username, password;
        iss >> username >> password;
        std::cout << "username: " << username << std::endl;
        std::cout << "password: " << password << std::endl;
        AsyncRegister( username, password );
    }
    else if ( command == "login" )
    {
        std::string username, password;
        iss >> username >> password;
        AsyncLogin( username, password );
    }
    else
    {
        socket_.close();
    }
}

void AuthSession::AsyncLogin( const std::string& username, const std::string& password )
{
    try
    {
        pqxx::work txn( conn_ );
        pqxx::result res = txn.exec_prepared( "authenticate_user", username, password );

        if ( res.empty() )
        {
            boost::asio::async_write( socket_, boost::asio::buffer( "Authentication failed\n" ),
                [this]( boost::system::error_code ec, std::size_t /*length*/ ) {
                    if ( !ec )
                    {
                        socket_.close();
                    }
                } );
        }
        else
        {
            boost::asio::async_write( socket_, boost::asio::buffer( "Authentication successful\n" ),
                [this, username]( boost::system::error_code ec, std::size_t /*length*/ ) {
                    if ( !ec )
                    {
                        auto chat_session =
                            std::make_shared<ChatSession>( std::move( socket_ ), server_ );
                        chat_session->Start();
                    }
                } );
        }

        txn.commit();
    }
    catch ( const std::exception& e )
    {
        boost::asio::async_write( socket_,
            boost::asio::buffer( std::string( "Error: " ) + e.what() + "\n" ),
            [this]( boost::system::error_code ec, std::size_t /*length*/ ) {
                if ( !ec )
                {
                    socket_.close();
                }
            } );
    }
}

void AuthSession::AsyncRegister( const std::string& username, const std::string& password )
{
    try
    {
        pqxx::work txn( conn_ );
        txn.exec_prepared( "register_user", username, password );
        txn.commit();

        boost::asio::async_write( socket_, boost::asio::buffer( "Registration successful\n" ),
            [this, username]( boost::system::error_code ec, std::size_t /*length*/ ) {
                if ( !ec )
                {
                    auto chat_session =
                        std::make_shared<ChatSession>( std::move( socket_ ), server_ );
                    chat_session->Start();
                }
            } );
    }
    catch ( const std::exception& e )
    {
        boost::asio::async_write( socket_,
            boost::asio::buffer( std::string( "Error: " ) + e.what() + "\n" ),
            [this]( boost::system::error_code ec, std::size_t /*length*/ ) {
                if ( !ec )
                {
                    socket_.close();
                }
            } );
    }
}

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

ChatServer::ChatServer(
    boost::asio::io_context& io_context, const tcp::endpoint& endpoint, pqxx::connection& conn )
    : io_context_( io_context ), acceptor_( io_context, endpoint ), isClose( false ), conn_( conn )
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
    std::lock_guard lock( mutex_ );

    participants_.push_back( participant );
    DEBUG_PRINT( "Participant added, current size: " << participants_.size() );
}

void ChatServer::RemoveParticipant( std::shared_ptr<ChatSession> participant )
{
    std::lock_guard lock( mutex_ );

    participants_.erase( std::remove( participants_.begin(), participants_.end(), participant ),
        participants_.end() );
    DEBUG_PRINT( "Participant removed, current size: " << participants_.size() );
}

void ChatServer::Accept()
{
    acceptor_.async_accept( [this]( boost::system::error_code ec, tcp::socket socket ) {
        if ( !ec )
        {
            auto auth_session = std::make_shared<AuthSession>( std::move( socket ), *this, conn_ );
            auth_session->Start();
        }
        Accept();
    } );
}

void ChatServer::Close()
{
    std::lock_guard lock( mutex_ );

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
    conn_.disconnect();
}

} // namespace server
