project(discoverage)

cmake_minimum_required(VERSION 2.6)

set(discoverage_SRCS
  main.cpp
  mainwindow.cpp
  scene.cpp
  cell.cpp
  gridmap.cpp
  toolhandler.cpp
  discoveragehandler.cpp
)

# another list, this time it includes all header files that should be treated with moc
set( discoverage_MOC_HDRS
  mainwindow.h
  scene.h
  discoveragehandler.h
)

# some .ui files
set( discoverage_UIS
  mainwindow.ui
  toolwidget.ui
  newscenedialog.ui
  discoveragewidget.ui
)

# and finally an resource file
set( discoverage_RCS
  app.qrc
)

# enable warnings
add_definitions( -Wall )

# by default only QtCore and QtGui modules are enabled
# other modules must be enabled like this:
#set( QT_USE_QT3SUPPORT TRUE )
#set( QT_USE_QTXML TRUE )

# this command finds Qt4 libraries and sets all required variables
# note that it's Qt4, not QT4 or qt4

find_package(Qt4 REQUIRED)

# add some useful macros and variables
# (QT_USE_FILE is a variable defined by FIND_PACKAGE( Qt4 ) that contains a path to CMake script)
INCLUDE( ${QT_USE_FILE} )

# this command will generate rules that will run rcc on all files from SAMPLE_RCS
# in result SAMPLE_RC_SRCS variable will contain paths to files produced by rcc
QT4_ADD_RESOURCES( discoverage_RC_SRCS ${discoverage_RCS} )

# this will run uic on .ui files:
QT4_WRAP_UI( discoverage_UI_HDRS ${discoverage_UIS} )

# and finally this will run moc:
QT4_WRAP_CPP( discoverage_MOC_SRCS ${discoverage_MOC_HDRS} )

# we need this to be able to include headers produced by uic in our code
# (CMAKE_BINARY_DIR holds a path to the build directory, while INCLUDE_DIRECTORIES() works just like INCLUDEPATH from qmake)
include_directories( ${CMAKE_BINARY_DIR} )

# here we instruct CMake to build "sample" executable from all of the source files
add_executable( discoverage ${discoverage_SRCS} ${discoverage_MOC_SRCS} ${discoverage_UIS} ${discoverage_RC_SRCS})

# last thing we have to do is to tell CMake what libraries our executable needs,
# luckily FIND_PACKAGE prepared QT_LIBRARIES variable for us:
target_link_libraries( discoverage ${QT_LIBRARIES} )
