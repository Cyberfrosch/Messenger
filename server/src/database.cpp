#include "database.hpp"

namespace server
{

#ifdef DEBUG
void PrintResult( const pqxx::result& result )
{
     std::stringstream ss;

     if ( result.empty() )
     {
          DEBUG_PRINT( "Query result is empty." );
          return;
     }

     for ( const auto& row : result )
     {
          for ( int i = 0; i < row.size(); ++i )
          {
               const auto& field = row.at( i );
               const auto& columnName = result.column_name( i );
               ss << columnName << ":" << field.c_str();
               if ( i < row.size() - 1 )
               {
                    ss << " "; // Separator between the lines
               }
          }
          ss << "\n";
     }
     DEBUG_PRINT( "Query result: " << ss.str() );
}
#endif

Database::Database( const std::string& connStr, std::size_t poolSize ) : connStr_( connStr )
{
     for ( std::size_t i = 0; i < poolSize; ++i )
     {
          auto conn = std::make_shared<pqxx::connection>( connStr_ );
          if ( !conn->is_open() )
          {
               throw std::runtime_error( "Failed to open database connection." );
          }
          PrepareStatements( conn );
          pool_.push( conn );
          DEBUG_PRINT( "Connection numbers: " << i + 1 << " success" << std::endl );
     }
}

pqxx::result Database::ExecQuery( const std::string& query )
{
     auto conn = GetConnection();
     pqxx::work txn( *conn );
     pqxx::result result = txn.exec( query );
     txn.commit();
     DEBUG_PRINT( "Read result of the query: " << query << " success" << std::endl );
     PrintResult( result );

     FreeConnection( conn );

     return result;
}

void Database::PrepareStatements( std::shared_ptr<pqxx::connection> conn )
{
     try
     {
          if ( conn && conn->is_open() )
          {
               pqxx::work txn( *conn );

               conn->prepare(
                    "register_user", "INSERT INTO users (username, password) VALUES ($1::VARCHAR, $2::VARCHAR)" );
               conn->prepare( "authenticate_user",
                    "SELECT * FROM users WHERE username = $1::VARCHAR AND password = $2::VARCHAR" );

               txn.commit();
          }
          else
          {
               throw std::runtime_error( "Connection is not open" );
          }
     }
     catch ( const std::exception& e )
     {
          std::cerr << "Failed to prepare statements: " << e.what() << std::endl;
          throw;
     }
}

std::shared_ptr<pqxx::connection> Database::GetConnection()
{
     std::unique_lock lock( mutex_ );
     condition_.wait( lock, [this]() { return !pool_.empty(); } );

     auto conn = pool_.front();
     pool_.pop();
     DEBUG_PRINT( "GetConnection() success. Size of connection pool now: " << pool_.size() << std::endl );

     return conn;
}

void Database::FreeConnection( std::shared_ptr<pqxx::connection> conn )
{
     std::unique_lock lock( mutex_ );

     pool_.push( conn );
     condition_.notify_one();
     DEBUG_PRINT( "FreeConnection() success. Size of connection pool now: " << pool_.size() << std::endl );
}

} // namespace server
