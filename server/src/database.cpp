#include "database.hpp"

namespace server
{

Database::Database( std::string_view connStr, const std::size_t poolSize )
: connStr_( connStr )
{
     for ( std::size_t i = 0; i < poolSize; ++i )
     {
          auto conn = std::make_shared<pqxx::connection>( connStr_ );
          if ( !conn->is_open() )
          {
               throw std::runtime_error( "Failed to open database connection" );
          }
          PrepareStatements( conn );
          pool_.push( conn );
          common::DebugPrint( "Connection {} success\n", i + 1 );
     }
}

pqxx::result Database::ExecQuery( std::string_view query )
{
     common::DebugPrint("{}:{}\n", __func__, __LINE__);
     auto conn = GetConnection();
     common::DebugPrint("{}:{}\n", __func__, __LINE__);
     pqxx::work txn( *conn );
     common::DebugPrint("{}:{}\n", __func__, __LINE__);
     pqxx::result result = txn.exec( std::string( query ) );
     txn.commit();
     common::DebugPrint( "Query executed: {} success\n", query );
     PrintResult( result );
     FreeConnection( conn );

     return result;
}

void Database::PrepareStatements( const std::shared_ptr<pqxx::connection>& conn ) const
{
     if ( !conn || !conn->is_open() )
     {
          throw std::runtime_error( "Connection is not open" );
     }

     common::DebugPrint("{} Preparing statements\n", __LINE__);
     pqxx::work txn( *conn );
     conn->prepare( std::string( db_statements::registerUser ),
          "INSERT INTO users (username, password) VALUES ($1::VARCHAR, $2::VARCHAR)" );
     conn->prepare( std::string( db_statements::authenticateUser ),
          "SELECT * FROM users WHERE username = $1::VARCHAR AND password = $2::VARCHAR" );
     txn.commit();
     common::DebugPrint("{} Preparing statements\n", __LINE__);
}

std::shared_ptr<pqxx::connection> Database::GetConnection()
{
     std::unique_lock lock( mutex_ );
     cv_.wait( lock, [this] { return !pool_.empty(); } );
     auto conn = pool_.front();
     pool_.pop();
     common::DebugPrint( "{} success. Pool size: {}\n", __func__, pool_.size() );

     return conn;
}

void Database::FreeConnection( const std::shared_ptr<pqxx::connection>& conn )
{
     std::unique_lock lock( mutex_ );
     pool_.push( conn );
     cv_.notify_one();
     common::DebugPrint( "{} success. Pool size: {}\n", __func__, pool_.size() );
}

} // namespace server
