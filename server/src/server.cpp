#include "server.hpp"

namespace server
{

ClientConnection::ClientConnection( tcp::socket socket, std::shared_ptr<Server> server )
    : socket_( std::move( socket ) ), server_( std::move( server ) )
{
}

void ClientConnection::Start()
{
     RequestIdentUser();
}

void ClientConnection::Deliver( const std::string& msg )
{
     bool write_in_progress = !writeMessages_.empty();
     writeMessages_.push_back( msg );
     DEBUG_PRINT( "Message added to write queue: " << msg );
     if ( !write_in_progress )
     {
          Write();
     }
}

void ClientConnection::Close()
{
     socket_.close();
}

void ClientConnection::Read()
{
     auto self( shared_from_this() );

     boost::asio::async_read_until( socket_, boost::asio::dynamic_buffer( data_ ), "\n",
          [this, self]( boost::system::error_code ec, std::size_t length ) {
               if ( !ec )
               {
                    std::string msg( data_.substr( 0, length ) );
                    DEBUG_PRINT( "Message received: " << msg );
                    if ( session_ )
                    {
                         session_->Deliver( msg );
                    }
                    data_.erase( 0, length );
                    Read();
               }
               else
               {
                    socket_.close();
               }
          } );
}

void ClientConnection::Write()
{
     auto self( shared_from_this() );

     boost::asio::async_write( socket_, boost::asio::buffer( writeMessages_.front() ),
          [this, self]( boost::system::error_code ec, [[maybe_unused]] std::size_t length ) {
               if ( !ec )
               {
                    DEBUG_PRINT( "Message sent: " << writeMessages_.front() );
                    writeMessages_.pop_front();
                    if ( !writeMessages_.empty() )
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

void ClientConnection::RequestSessionId()
{
     auto self( shared_from_this() );

     boost::asio::async_write( socket_, boost::asio::buffer( "Enter chat session ID (or 0 to create new session):\n" ),
          [this, self]( boost::system::error_code ec, [[maybe_unused]] std::size_t length ) {
               if ( !ec )
               {
                    ReadSessionId();
               }
          } );
}

void ClientConnection::ReadSessionId()
{
     auto self( shared_from_this() );

     boost::asio::async_read_until( socket_, inputBuffer_, '\n',
          [this, self]( boost::system::error_code ec, [[maybe_unused]] std::size_t length ) {
               if ( !ec )
               {
                    std::istream is( &inputBuffer_ );
                    int id;
                    is >> id;
                    inputBuffer_.consume( inputBuffer_.size() );

                    JoinChat( id );
               }
               else
               {
                    socket_.close();
               }
          } );
}

void ClientConnection::JoinChat( int id )
{
     if ( id == 0 )
     {
          int newSessionId = server_->CreateSession();
          session_ = server_->GetSession( newSessionId );
          if ( session_ )
          {
               session_->Join( shared_from_this() );
               auto self( shared_from_this() );
               boost::asio::async_write( socket_,
                    boost::asio::buffer( "New chat session created with ID: " + std::to_string( newSessionId ) + "\n" ),
                    [this, self]( boost::system::error_code ec, [[maybe_unused]] std::size_t length ) {
                         if ( !ec )
                         {
                              Read();
                         }
                    } );
          }
     }
     else
     {
          session_ = server_->GetSession( id );
          if ( session_ )
          {
               session_->Join( shared_from_this() );
               Read();
          }
          else
          {
               auto self( shared_from_this() );
               boost::asio::async_write( socket_, boost::asio::buffer( "Invalid chat session ID\n" ),
                    [this, self]( boost::system::error_code ec, [[maybe_unused]] std::size_t length ) {
                         if ( !ec )
                         {
                              RequestSessionId();
                         }
                    } );
          }
     }
}

void ClientConnection::RequestIdentUser()
{
     auto self( shared_from_this() );

     boost::asio::async_write( socket_,
          boost::asio::buffer( "Please sign up or sign in (use \"REG\" or \"AUTH\" <username> <password>):\n" ),
          [this, self]( boost::system::error_code ec, [[maybe_unused]] std::size_t length ) {
               if ( !ec )
               {
                    ReadIdentUser();
               }
          } );
}

void ClientConnection::ReadIdentUser()
{
     auto self( shared_from_this() );

     boost::asio::async_read_until( socket_, boost::asio::dynamic_buffer( data_ ), "\n",
          [this, self]( boost::system::error_code ec, std::size_t length ) {
               if ( !ec )
               {
                    std::string msg( data_.substr( 0, length ) );
                    data_.erase( 0, length );

                    if ( msg.rfind( "AUTH", 0 ) == 0 )
                    {
                         std::istringstream iss( msg.substr( 5 ) );
                         std::string username, password;
                         if ( iss >> username >> password )
                         {
                              AuthUser( username, password );
                         }
                         else
                         {
                              Deliver( "Invalid AUTH command format!\n" );
                         }
                    }
                    else if ( msg.rfind( "REG", 0 ) == 0 )
                    {
                         std::istringstream iss( msg.substr( 4 ) );
                         std::string username, password;
                         if ( iss >> username >> password )
                         {
                              RegisterUser( username, password );
                         }
                         else
                         {
                              Deliver( "Invalid REG command format!\n" );
                         }
                    }
                    else
                    {
                         Deliver( "You must previously sign up or sign in (use \"REG\" or \"AUTH\" "
                                  "<username> <password>)!\n" );
                         ReadIdentUser();
                    }
               }
               else
               {
                    socket_.close();
               }
          } );
}

void ClientConnection::RegisterUser( const std::string& username, const std::string& password )
{
     auto db = server_->GetDatabase();
     pqxx::result result = db->ExecPreparedQuery( "authenticate_user", username, password );

     if ( !result.empty() )
     {
          Deliver( "User already exists!\n" );
          RequestIdentUser();
          return;
     }

     db->ExecPreparedQuery( "register_user", username, password );

     Deliver( "Registration successful\n" );
     RequestSessionId();
}

void ClientConnection::AuthUser( const std::string& username, const std::string& password )
{
     auto db = server_->GetDatabase();
     pqxx::result result = db->ExecPreparedQuery( "authenticate_user", username, password );

     if ( result.size() == 1 )
     {
          Deliver( "Authentication successful\n" );
          RequestSessionId();
     }
     else
     {
          Deliver( "Authentication failed!\n" );
          Close();
     }
}

Session::Session( int id ) : id_( id )
{
}

void Session::Join( std::shared_ptr<ClientConnection> clientConn )
{
     std::lock_guard lock( mutex_ );

     clientsConn_.insert( clientConn );
}

void Session::Leave( std::shared_ptr<ClientConnection> clientConn )
{
     std::lock_guard lock( mutex_ );

     clientsConn_.erase( clientConn );
}

void Session::Deliver( const std::string& msg )
{
     std::lock_guard lock( mutex_ );

     for ( const auto& clientConn : clientsConn_ )
     {
          clientConn->Deliver( msg );
     }
}

void Session::Close()
{
     std::lock_guard lock( mutex_ );

     for ( const auto& clientConn : clientsConn_ )
     {
          clientConn->Close();
     }
     clientsConn_.clear();
}

Server::Server( boost::asio::io_context& io_context, const tcp::endpoint& endpoint, const std::string& connStr,
     std::size_t connSize )
    : io_context_( io_context ), acceptor_( io_context, endpoint ), isClose_( false ),
      db_( std::make_shared<Database>( connStr, connSize ) )
{
     Accept();
}

Server::~Server()
{
     std::cout << "Server has been closed" << std::endl;
     Close();
}

int Server::CreateSession()
{
     std::lock_guard lock( mutex_ );
     int id = 1;

     while ( sessions_.find( id ) != sessions_.end() )
     {
          id++;
     }
     sessions_.try_emplace( id, std::make_shared<Session>( id ) );

     return id;
}

std::shared_ptr<Session> Server::GetSession( int id )
{
     std::lock_guard lock( mutex_ );

     auto it = sessions_.find( id );
     if ( it != sessions_.end() )
     {
          return it->second;
     }

     return nullptr;
}

std::shared_ptr<Database> Server::GetDatabase() const
{
     return db_;
}

void Server::Accept()
{
     acceptor_.async_accept( [this]( boost::system::error_code ec, tcp::socket socket ) {
          if ( !ec )
          {
               auto newConnection = std::make_shared<ClientConnection>( std::move( socket ), shared_from_this() );
               newConnection->Start();
          }
          Accept();
     } );
}

void Server::Close()
{
     if ( isClose_ )
     {
          return;
     }

     std::lock_guard lock( mutex_ );

     DEBUG_PRINT( "Sessions size before close: " << sessions_.size() << std::endl );
     for ( auto& [id, session] : sessions_ )
     {
          session->Deliver( "Server is shutting down\n" );
          session->Close();
     }
     sessions_.clear();
     DEBUG_PRINT( "Sessions size after close: " << sessions_.size() << std::endl );

     acceptor_.close();
     io_context_.stop();
     isClose_ = true;
}

} // namespace server
