project(keys)

file(GLOB PEMS *.pem)
file(COPY ${PEMS} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)
