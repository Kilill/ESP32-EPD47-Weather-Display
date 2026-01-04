idf_component_register(
    SRC_DIRS "src" "src/zlib"
    INCLUDE_DIRS "src" "src/zlib"
    PRIV_REQUIRES esp_lcd driver
    EXCLUDE_SRCS "src/libjpeg/libjpeg.c"
)
