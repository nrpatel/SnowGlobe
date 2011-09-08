SET(SRC 
    osgsnowglobe.cpp
)

INCLUDE_DIRECTORIES(${OPENSCENEGRAPH_INCLUDE_DIR} ${OPENTHREADS_INCLUDE_DIR})

IF(DEBUG)
    LINK_DIRECTORIES($ENV{OPENSCENEGRAPH_LIBRARY_DIR_DEBUG})
ELSE(DEBUG)
    LINK_DIRECTORIES($ENV{OPENSCENEGRAPH_LIBRARY_DIR})
ENDIF(DEBUG)


SET(TARGET_SRC ${SRC})

SET(TARGET_NAME osgsnowglobe)
ADD_EXECUTABLE(${TARGET_NAME} ${TARGET_SRC} ${TARGET_H})
SET_TARGET_PROPERTIES(${TARGET_NAME} PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})

# The libraries have different names in release as in debug.
TARGET_LINK_LIBRARIES(${TARGET_NAME}
                      debug ${OPENTHREADS_LIBRARY_DEBUG}
                      debug osgd 
                      debug osgDBd 
                      debug osgGAd 
                      debug osgViewerd 
                      debug osgUtild
                      debug osgFXd
                      debug osgTextd
                      debug osgManipulatord
                      debug osgEarthd
                      debug osgEarthUtild
                      debug osgEarthSymbologyd
                      optimized ${OPENTHREADS_LIBRARY}
                      optimized osg 
                      optimized osgDB 
                      optimized osgGA 
                      optimized osgViewer 
                      optimized osgUtil
                      optimized osgFX
                      optimized osgText
                      optimized osgManipulator
                      optimized osgEarth
                      optimized osgEarthUtil
                      optimized osgEarthSymbology
                     )

INSTALL(TARGETS ${TARGET_NAME}
        RUNTIME DESTINATION bin)
