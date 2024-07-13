#pragma once

#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <pqxx/pqxx>

#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>

#include "common.hpp"

namespace server
{

#ifdef DEBUG
void PrintResult( const pqxx::result& result );

template <typename T>
std::string ToString( const T& value )
{
     std::ostringstream oss;
     oss << value;
     return oss.str();
}

template <typename... Args>
std::string ArgsToString( Args&&... args )
{
     std::ostringstream oss;
     ( ( oss << "\"" << ToString( args ) << "\""
             << " " ),
          ... );
     return oss.str();
}
#else
#define PrintResult( ... ) NULL
#define ToString( ... ) NULL
#define ArgsToString( ... ) NULL
#endif // DEBUG

namespace db_statements
{

constexpr auto authenticateUser = "authenticate_user";
constexpr auto registerUser = "register_user";

} // namespace db_statements

class Database
{
public:
     Database( std::string_view connStr, const std::size_t& poolSize );

     pqxx::result ExecQuery( const std::string& query );
     template <typename... Args>
     pqxx::result ExecPreparedQuery( const std::string& stmt, Args&&... args )
     {
          auto conn = GetConnection();
          pqxx::work txn( *conn );
          pqxx::result result = txn.exec_prepared( stmt, std::forward<Args>( args )... );
          txn.commit();
          DEBUG_PRINT( "Read result of the prepared statement: "
                       << "\"" << stmt << "\""
                       << " with args: " << ArgsToString( std::forward<Args>( args )... ) << "success" << std::endl );
          PrintResult( result );
          FreeConnection( conn );

          return result;
     }

     void PrepareStatements( const std::shared_ptr<pqxx::connection>& conn ) const;

private:
     std::shared_ptr<pqxx::connection> GetConnection();
     void FreeConnection( const std::shared_ptr<pqxx::connection>& conn );

     std::string connStr_;
     std::queue<std::shared_ptr<pqxx::connection>> pool_;

     std::mutex mutex_;
     std::condition_variable cv_;
};

} // namespace server

#endif // DATABASE_HPP
