INCLUDE_DIRECTORIES(${urilite_SOURCE_DIR}/include)

FILE(GLOB src_files ${urilite_SOURCE_DIR}/src/*)
ADD_LIBRARY(urilite SHARED ${src_files})
ADD_LIBRARY(urilite_static STATIC ${src_files})
# SET_TARGET_PROPERTIES(urilite_static PROPERTIES OUTPUT_NAME urilite)

FILE(GLOB header_files ${urilite_SOURCE_DIR}/include/urilite/*)
INSTALL(FILES ${header_files} DESTINATION include/urilite)
INSTALL(TARGETS urilite DESTINATION lib)
