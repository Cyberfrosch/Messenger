#include "server.hpp"

std::vector<std::shared_ptr<ChatSession>> ChatSession::participants_;

void ChatSession::Start()
{
    Read();
}

void ChatSession::Deliever( const std::string& msg )
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

    boost::asio::async_read_until(
        socket_, boost::asio::dynamic_buffer( buffer_ ), "\n",
        [this, self]( boost::system::error_code ec, std::size_t length ) {
            if ( !ec )
            {
                std::string msg( buffer_.substr( 0, length ) );
                buffer_.erase( 0, length );
                DelieverToAll( msg );
                Read();
            }
            else
            {
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
                                        socket_.close();
                                    }
                                } );
}

void ChatSession::DelieverToAll( const std::string& msg )
{
    for ( auto& participant : participants_ )
    {
        participant->Deliever( msg );
    }
}

void ChatServer::accept()
{
    acceptor_.async_accept( [this]( boost::system::error_code ec, tcp::socket socket ) {
        if ( !ec )
        {
            auto session = std::make_shared<ChatSession>( std::move( socket ) );
            ChatSession::participants_.push_back( session );
            session->Start();
        }
        accept();
    } );
}

int main( int argc, char* argv[] )
{
    try
    {
        if ( argc != 2 )
        {
            std::cerr << "Usage: chat_server <port>" << std::endl;
            return 1;
        }

        boost::asio::io_context io_context;
        tcp::endpoint endpoint( tcp::v4(), std::atoi( argv[1] ) );
        ChatServer server( io_context, endpoint );
        io_context.run();
    }
    catch ( std::exception& e )
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
