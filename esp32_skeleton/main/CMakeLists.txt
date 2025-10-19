idf_component_register(SRCS "main.c"
                          "ds18b20.c" 
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES driver
                    )