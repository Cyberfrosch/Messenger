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
std::string ArgsToStirng( Args&&... args )
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
#define ArgsToStirng( ... ) NULL
#endif // DEBUG

class Database
{
public:
     Database( const std::string& connStr, std::size_t poolSize );

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
                       << " with args: " << ArgsToStirng( std::forward<Args>( args )... ) << "success" << std::endl );
          PrintResult( result );
          FreeConnection( conn );

          return result;
     }

     void PrepareStatements( std::shared_ptr<pqxx::connection> conn );

private:
     std::shared_ptr<pqxx::connection> GetConnection();
     void FreeConnection( std::shared_ptr<pqxx::connection> conn );

     std::string connStr_;
     std::queue<std::shared_ptr<pqxx::connection>> pool_;

     std::mutex mutex_;
     std::condition_variable condition_;
};

} // namespace server

#endif // DATABASE_HPP
