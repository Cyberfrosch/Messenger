#pragma once

#include <condition_variable>
#include <format>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <string_view>
#include <ranges>

#include "common.hpp"

#include <pqxx/pqxx>

namespace server
{

inline void PrintResult( const pqxx::result& result )
{
     if constexpr ( common::isDebug )
     {
          if ( result.empty() )
          {
               common::DebugPrint( "Query result is empty\n" );
               return;
          }
          for ( const auto& row : result )
          {
               for ( pqxx::row::size_type i = 0; i < row.size(); ++i )
               {
                    common::DebugPrint( "{}:{} ", result.column_name( i ), row[i].c_str() );
               }
               common::DebugPrint( "\n" );
          }
     }
}

namespace db_statements
{

inline constexpr std::string_view authenticateUser = "authenticate_user";
inline constexpr std::string_view registerUser = "register_user";

} // namespace db_statements

class Database
{
public:
     Database( std::string_view connStr, const std::size_t poolSize );

     pqxx::result ExecQuery( std::string_view query );
     void PrepareStatements( const std::shared_ptr<pqxx::connection>& conn ) const;

public:
     template <typename... Args>
     pqxx::result ExecPreparedQuery( std::string_view stmt, Args&&... args )
     {
          auto conn = GetConnection();
          pqxx::work txn( *conn );
          // TODO: Исправить вызов подготовленного запроса с передачей fold-expression
          // pqxx::result result = txn.exec( stmt, pqxx::params{ std::forward<Args>( args )... } );
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
          pqxx::result result = txn.exec_prepared( std::string( stmt ), std::forward<Args>( args )... );
#pragma GCC diagnostic pop
          txn.commit();

          if constexpr ( common::isDebug )
          {
               std::ostringstream oss;
               bool first = true;

               auto append_arg = [&]( auto&& arg ) {
                    if ( !first )
                         oss << ", ";
                    first = false;
                    oss << arg;
               };

               ( append_arg( std::forward<Args>( args ) ), ... );

               common::DebugPrint( "Prepared statement \"{}\" executed with args: [{}] success\n", stmt, oss.str() );
          }

          PrintResult( result );
          FreeConnection( conn );
          return result;
     }

private:
     std::shared_ptr<pqxx::connection> GetConnection();
     void FreeConnection( const std::shared_ptr<pqxx::connection>& conn );

private:
     std::string connStr_;
     std::queue<std::shared_ptr<pqxx::connection>> pool_;
     std::mutex mutex_;
     std::condition_variable cv_;
};

} // namespace server
