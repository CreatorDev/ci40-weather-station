CMAKE_MINIMUM_REQUIRED(VERSION 3.5)

include_directories(${STAGING_DIR} ${STAGING_DIR}/usr/include/glib-2.0 ${STAGING_DIR}/usr/include/curl)
# Add executable targets
########################
ADD_EXECUTABLE(weatherStation dumpReading.c wu.c)

# Add library targets
#####################
FIND_LIBRARY(LIB_AWA libawa.so ${STAGING_DIR}/usr/lib)
FIND_LIBRARY(LIB_GLIB libglib-2.0.so ${STAGING_DIR}/usr/lib)
FIND_LIBRARY(LIB_CURL libcurl.so ${STAGING_DIR}/usr/lib)
FIND_PACKAGE(LetMeCreate CONFIG REQUIRED)
TARGET_LINK_LIBRARIES(weatherStation letmecreate_core letmecreate_click ${LIB_AWA} ${LIB_GLIB} ${LIB_CURL})

# Add install targets
######################
INSTALL(TARGETS weatherStation RUNTIME DESTINATION bin)
