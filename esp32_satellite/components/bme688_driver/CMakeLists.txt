# Adjust this to include all the .c files for the bme driver
idf_component_register(SRCS "bme688.c" "bme68x.c"
                      INCLUDE_DIRS ".")