# Collect all .c files from the src directory
file(GLOB SRCS "main.c" "src/*.c")

idf_component_register(
    SRCS ${SRCS}  # Use the collected source files
    INCLUDE_DIRS "include"  # Specify the include directory
)
