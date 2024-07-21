add_library(pico_con INTERFACE)
target_sources(pico_con INTERFACE
	${CMAKE_CURRENT_LIST_DIR}/pico-con.c
	)
target_include_directories(pico_con INTERFACE
	${CMAKE_CURRENT_LIST_DIR}
	)
target_link_libraries(pico_con INTERFACE
	pico_stdlib
	)
