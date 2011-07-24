cmake_minimum_required(VERSION 2.6)

project(ChaiGame)


find_package(SDL)
find_package(SDL_image)

IF(MSVC)
  ADD_DEFINITIONS(/W4)
  IF(CMAKE_CL_64)
    ADD_DEFINITIONS(/bigobj)
   ENDIF()
ELSE()
  ADD_DEFINITIONS(-Wall -Wextra -Wshadow) 

  IF (APPLE)
    # -Wno-missing-field-initializers is for boost on macos
    ADD_DEFINITIONS(-Wno-missing-field-initializers -Wno-sign-compare)
  ENDIF()
ENDIF()


add_executable(chaigame main.cpp)

target_link_libraries(chaigame ${SDL_LIBRARY} ${SDLIMAGE_LIBRARY} )
