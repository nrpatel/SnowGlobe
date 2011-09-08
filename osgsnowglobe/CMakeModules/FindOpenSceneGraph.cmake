# Locate OpenSceneGraph
# This module defines
# OPENSCENEGRAPH_LIBRARY_DIR
# OPENSCENEGRAPH_LIBRARY_DIR_DEBUG
# OPENSCENEGRAPH_FOUND, if false, do not try to link to OpenSceneGraph
# OPENSCENEGRAPH_INCLUDE_DIR, where to find the headers

# Header files are presumed to be included like
# #include <osg/Node>

# To make it easier for one-step automated configuration/builds,
# we leverage environmental paths. This is preferable
# to the -DVAR=value switches because it insulates the
# users from changes we may make in this script.
# It also offers a little more flexibility than setting
# the CMAKE_*_PATH since we can target specific components.
# However, the default CMake behavior will search system paths
# before anything else. This is problematic in the cases
# where you have an older (stable) version installed, but
# are trying to build a newer version.
# CMake doesn't offer a nice way to globally control this behavior
# so we have to do a nasty "double FIND_" in this module.
# The first FIND disables the CMAKE_ search paths and only checks
# the environmental paths.
# If nothing is found, then the second find will search the
# standard install paths.
# Explicit -DVAR=value arguments should still be able to override everything.
# Note: We have added an additional check for ${CMAKE_PREFIX_PATH}.
# This is not an official CMake variable, but one we are proposing be
# added to CMake. Be warned that this may go away or the variable name
# may change.


# Find the include path.
FIND_PATH(OPENSCENEGRAPH_INCLUDE_DIR osg/Node
    $ENV{OPENSCENEGRAPH_INCLUDE_DIR}
    $ENV{OPENSCENEGRAPH_DIR}/include
    $ENV{OPENSCENEGRAPH_DIR}
    $ENV{OSG_INCLUDE_DIR}
    $ENV{OSG_INCLUDE_PATH}
    $ENV{OSG_DIR}/include
    $ENV{OSG_DIR}
    NO_DEFAULT_PATH
)

IF(NOT OPENSCENEGRAPH_INCLUDE_DIR)
    FIND_PATH(OPENSCENEGRAPH_INCLUDE_DIR osg/Node
        PATHS ${CMAKE_PREFIX_PATH} # Unofficial: We are proposing this.
        PATH_SUFFIXES include
    )
ENDIF(NOT OPENSCENEGRAPH_INCLUDE_DIR)

IF(NOT OPENSCENEGRAPH_INCLUDE_DIR)
    FIND_PATH(OPENSCENEGRAPH_INCLUDE_DIR osg/Node
        ~/Library/Frameworks
        /Library/Frameworks
        /usr/local/include
        /usr/include
        /sw/include # Fink
        /opt/local/include # DarwinPorts
        /opt/csw/include # Blastwave
        /opt/include
        [HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Session\ Manager\\Environment;OpenSceneGraph_ROOT]/include
        [HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Session\ Manager\\Environment;OSG_ROOT]/include
    )
ENDIF(NOT OPENSCENEGRAPH_INCLUDE_DIR)


# Find the library path for release mode libraries.
FIND_PATH(OPENSCENEGRAPH_LIBRARY_DIR
    NAMES
    osg.lib libosg.so
    PATHS
    $ENV{OPENSCENEGRAPH_LIBRARY_DIR}
    $ENV{OPENSCENEGRAPH_DIR}/lib64
    $ENV{OPENSCENEGRAPH_DIR}/lib
    $ENV{OPENSCENEGRAPH_DIR}
    $ENV{OSG_LIBRARY_DIR}
    $ENV{OSG_LIB_PATH}
    $ENV{OSG_DIR}/lib64
    $ENV{OSG_DIR}/lib
    $ENV{OSG_DIR}
    NO_DEFAULT_PATH
)

IF(NOT OPENSCENEGRAPH_LIBRARY_DIR)
    FIND_PATH(OPENSCENEGRAPH_LIBRARY_DIR
        NAMES
        osg.lib libosg.so
        PATHS
        ${CMAKE_PREFIX_PATH} # Unofficial: We are proposing this.
        PATH_SUFFIXES lib64 lib
    )
ENDIF(NOT OPENSCENEGRAPH_LIBRARY_DIR)

IF(NOT OPENSCENEGRAPH_LIBRARY_DIR)
    FIND_PATH(OPENSCENEGRAPH_LIBRARY_DIR
        NAMES
        osg.lib libosg.so
        PATHS
        ~/Library/Frameworks
        /Library/Frameworks
        /usr/local/lib64
        /usr/local/lib
        /usr/lib64
        /usr/lib
        /sw/lib64
        /sw/lib
        /opt/local/lib64
        /opt/local/lib
        /opt/csw/lib64
        /opt/csw/lib
        /opt/lib64
        /opt/lib
        [HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Session\ Manager\\Environment;OpenSceneGraph_ROOT]/lib
        [HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Session\ Manager\\Environment;OSG_ROOT]/lib
    )
ENDIF(NOT OPENSCENEGRAPH_LIBRARY_DIR)


# Find the library path for debug mode libraries.
FIND_PATH(OPENSCENEGRAPH_LIBRARY_DIR_DEBUG
    NAMES
    osgd.lib libosgd.so
    PATHS
    $ENV{OPENSCENEGRAPH_LIBRARY_DIR}
    $ENV{OPENSCENEGRAPH_DIR}/lib64
    $ENV{OPENSCENEGRAPH_DIR}/lib
    $ENV{OPENSCENEGRAPH_DIR}
    $ENV{OSG_LIBRARY_DIR}
    $ENV{OSG_LIB_PATH_DEBUG}
    $ENV{OSG_LIB_PATH}
    $ENV{OSG_DIR}/lib64
    $ENV{OSG_DIR}/lib
    $ENV{OSG_DIR}
    NO_DEFAULT_PATH
)

IF(NOT OPENSCENEGRAPH_LIBRARY_DIR_DEBUG)
    FIND_PATH(OPENSCENEGRAPH_LIBRARY_DIR_DEBUG
        NAMES
        osgd.lib libosgd.so
        PATHS
        ${CMAKE_PREFIX_PATH} # Unofficial: We are proposing this.
        PATH_SUFFIXES lib64 lib
    )
ENDIF(NOT OPENSCENEGRAPH_LIBRARY_DIR_DEBUG)

IF(NOT OPENSCENEGRAPH_LIBRARY_DIR_DEBUG)
    FIND_PATH(OPENSCENEGRAPH_LIBRARY_DIR_DEBUG
        NAMES
        osgd.lib libosgd.so
        PATHS
        ~/Library/Frameworks
        /Library/Frameworks
        /usr/local/lib64
        /usr/local/lib
        /usr/lib64
        /usr/lib
        /sw/lib64
        /sw/lib
        /opt/local/lib64
        /opt/local/lib
        /opt/csw/lib64
        /opt/csw/lib
        /opt/lib64
        /opt/lib
        [HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Session\ Manager\\Environment;OpenSceneGraph_ROOT]/lib
        [HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Session\ Manager\\Environment;OSG_ROOT]/lib
    )
ENDIF(NOT OPENSCENEGRAPH_LIBRARY_DIR_DEBUG)

# If the debug library path was not found, use the release versions.
IF(OPENSCENEGRAPH_LIBRARY_DIR)
  IF(NOT OPENSCENEGRAPH_LIBRARY_DIR_DEBUG)
      #MESSAGE("-- Warning Debug OpenSceneGraph not found, using: ${OPENSCENEGRAPH_LIBRARY_DIR}")
      #SET(OPENSCENEGRAPH_LIBRARY_DIR_DEBUG "${OPENSCENEGRAPH_LIBRARY_DIR}")
      SET(OPENSCENEGRAPH_LIBRARY_DIR_DEBUG "${OPENSCENEGRAPH_LIBRARY_DIR}" CACHE FILEPATH "Debug version of OpenSceneGraph Library (use regular version if not available)" FORCE)
  ENDIF(NOT OPENSCENEGRAPH_LIBRARY_DIR_DEBUG)
ENDIF(OPENSCENEGRAPH_LIBRARY_DIR)



SET(OPENSCENEGRAPH_FOUND "NO")
IF(OPENSCENEGRAPH_INCLUDE_DIR AND OPENSCENEGRAPH_LIBRARY_DIR)
  SET(OPENSCENEGRAPH_FOUND "YES")
  # MESSAGE("-- Found OpenSceneGraph: "${OPENSCENEGRAPH_LIBRARY_DIR})
ENDIF(OPENSCENEGRAPH_INCLUDE_DIR AND OPENSCENEGRAPH_LIBRARY_DIR)

