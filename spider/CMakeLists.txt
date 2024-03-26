cmake_minimum_required(VERSION 3.20)


add_executable(SpiderApp
	main.h
	main.cpp
	http_utils.h
	http_utils.cpp
	http_parser.h
	http_parser.cpp
	link.h
	../settings.h
	../settings.cpp
	../database.h
	../database.cpp
	)

target_compile_features(SpiderApp PRIVATE cxx_std_17) 

target_link_libraries(SpiderApp pqxx)

target_link_libraries(SpiderApp OpenSSL::SSL)

target_include_directories(SpiderApp PRIVATE ${Boost_INCLUDE_DIRS})

target_link_libraries(SpiderApp ${Boost_LIBRARIES})
