# Версия CMake
cmake_minimum_required(VERSION 3.14)

# Название приложения
set(TARGET_NAME "01_Basic")
set(TARGET_BIN_NAME "01_Basic")

# Добавляем .exe (проект в Visual Studio)
add_executable(${TARGET_NAME}
        "Main.cpp" "Types.h" "Sphere.hpp")

# Меняем название запускаемого файла в зависимости от типа сборки
set_property(TARGET ${TARGET_NAME} PROPERTY OUTPUT_NAME "${TARGET_BIN_NAME}$<$<CONFIG:Debug>:_Debug>_${PLATFORM_BIT_SUFFIX}")

# Статическая линковка рантайма и стандартных библиотек + флаги компиляции (показывать все warning'и)
if(MSVC)
    target_compile_definitions(${TARGET_NAME} PUBLIC "-DNOMINMAX")
    target_compile_options(${TARGET_NAME} PUBLIC /W3 /permissive-)
    set_property(TARGET ${TARGET_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
    target_compile_options(${TARGET_NAME} PUBLIC -Wall -Wextra -pedantic)
    set_property(TARGET ${TARGET_NAME} PROPERTY LINK_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive")
endif()

# Линковка со вспомогательной библиотекой (header-only)
target_link_libraries(${TARGET_NAME} PUBLIC "Common")