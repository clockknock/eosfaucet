project(eosfaucet)

cmake_minimum_required(VERSION 3.12)

include_directories(/usr/local/include)
include_directories(/usr/local/Cellar/eosio.cdt/1.4.1/opt/eosio.cdt/include)

add_executable(eosfaucet eosfaucet.cpp)