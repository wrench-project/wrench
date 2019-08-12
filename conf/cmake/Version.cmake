# configure wrench version
configure_file (
        "${PROJECT_SOURCE_DIR}/include/wrench/simulation/Version.h.in"
        "${PROJECT_SOURCE_DIR}/include/wrench/simulation/Version.h"
)
configure_file (
        "${PROJECT_SOURCE_DIR}/sonar-project.properties.in"
        "${PROJECT_SOURCE_DIR}/sonar-project.properties"
)
