cmake_minimum_required( VERSION 3.15 )

project( simple_mapper )

set( SOURCES
    src/json_data_source.cpp
    src/simple_mapper.cpp
)

add_executable( simple_mapper ${SOURCES} )
target_link_libraries( simple_mapper PRIVATE LibTokaMap::libtokamap )
