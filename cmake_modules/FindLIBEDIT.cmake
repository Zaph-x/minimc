FIND_PATH(LIBEDIT_INCLUDE_DIR NAMES histedit.h)
FIND_LIBRARY(LIBEDIT_LIBRARY NAMES edit)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LIBEDIT DEFAULT_MSG LIBEDIT_LIBRARY LIBEDIT_INCLUDE_DIR)

IF(LIBEDIT_FOUND)
	SET(LIBEDIT_LIBRARIES ${LIBEDIT_LIBRARY})
	SET(LIBEDIT_INCLUDE_DIRS ${LIBEDIT_INCLUDE_DIR})
ELSE(SQLITE3_FOUND)
	SET(LIBEDIT_LIBRARIES)
	SET(LIBEDIT_INCLUDE_DIRS)
ENDIF(LIBEDIT_FOUND)