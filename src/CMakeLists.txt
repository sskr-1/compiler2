set(SRC
  main.cpp
  lexer.cpp
  parser.cpp
  ast.cpp
  semantic.cpp
  irgen.cpp
)

add_executable(cmini ${SRC})

target_include_directories(cmini PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
