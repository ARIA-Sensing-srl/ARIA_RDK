cmake_minimum_required(VERSION 3.5)

project(ARIA_RDK VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets LinguistTools Sql SerialPort)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools Sql SerialPort)
find_package(JKQTPlotter${QT_VERSION_MAJOR} REQUIRED)
find_package(VTK REQUIRED)
# Find external library

set(TS_FILES ARIA_RDK_en_US.ts)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
#        ${TS_FILES}
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(ARIA_RDK
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        uiElements/mdioctaveinterface.h uiElements/mdioctaveinterface.cpp uiElements/mdioctaveinterface.ui
        uiElements/wndoctavescript.h uiElements/wndoctavescript.cpp uiElements/wndoctavescript.ui
        uiElements/wndradarmoduleeditor.h uiElements/wndradarmoduleeditor.cpp uiElements/wndradarmoduleeditor.ui
        octaveInterface/octaveinterface.h octaveInterface/octaveinterface.cpp
        radarModules/radarmodule.h radarModules/radarmodule.cpp

        octaveInterface/octavews.h octaveInterface/octavews.cpp

        radarModules/antenna.h radarModules/antenna.cpp
        radarModules/radarinstance.h radarModules/radarinstance.cpp
        radarModules/antennainstance.h radarModules/antennainstance.cpp
        uiElements/octavesyntaxhighlighter.h uiElements/octavesyntaxhighlighter.cpp

        uiElements/wnddefinecustomvar.h uiElements/wnddefinecustomvar.cpp uiElements/wnddefinecustomvar.ui

        controller/radarcontroller.h controller/radarcontroller.cpp
        projects/radarproject.h projects/radarproject.cpp
        uiElements/wndantennadesigner.h uiElements/wndantennadesigner.cpp uiElements/wndantennadesigner.ui
        octaveInterface/octavescript.h octaveInterface/octavescript.cpp
        utils/ariautils.cpp
        utils/ariautils.h
        radarModules/radarparameter.h
        radarModules/radarparameter.cpp
        serialcomm/radarcommthread.h serialcomm/radarcommthread.cpp
        uiElements/qdlgeditlimits.h uiElements/qdlgeditlimits.cpp uiElements/qdlgeditlimits.ui
        uiElements/wnddirectionfocusing.h uiElements/wnddirectionfocusing.cpp uiElements/wnddirectionfocusing.ui
        serialcomm/interfacecommand.h serialcomm/interfacecommand.cpp
        serialcomm/interfaceprotocol.h serialcomm/interfaceprotocol.cpp
        uiElements/wndradar.h uiElements/wndradar.cpp uiElements/wndradar.ui

        uiElements/wndscanmodules.h uiElements/wndscanmodules.cpp uiElements/wndscanmodules.ui
        utils/serialport_utils.h
        utils/serialport_utils.cpp

        uiElements/wndradarinstanceeditor.h uiElements/wndradarinstanceeditor.cpp uiElements/wndradarinstanceeditor.ui
        uiElements/wndimportradar.h uiElements/wndimportradar.cpp uiElements/wndimportradar.ui

        uiElements/wndplot2d.h uiElements/wndplot2d.cpp uiElements/wndplot2d.ui
        uiElements/plotdescriptor.h
        uiElements/wndradarliverun.h uiElements/wndradarliverun.cpp uiElements/wndradarliverun.ui
        uiElements/plotdescriptor.cpp
	uiElements/q_debugstream.h
	scheduler/opscheduler.h scheduler/opscheduler.cpp
	uiElements/wndscheduler.h uiElements/wndscheduler.cpp uiElements/wndscheduler.ui

	uiElements/wndabout.h uiElements/wndabout.cpp uiElements/wndabout.ui
	uiElements/wndfwupload.h uiElements/wndfwupload.cpp uiElements/wndfwupload.ui
	icons/aria_rdk.qrc
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET ARIA_RDK APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation

    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    if(ANDROID)
        add_library(ARIA_RDK SHARED
            ${PROJECT_SOURCES}

        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(ARIA_RDK
            ${PROJECT_SOURCES}
	    uiElements/mdioctaveinterface.h uiElements/mdioctaveinterface.cpp uiElements/mdioctaveinterface.ui
	    uiElements/wndoctavescript.h uiElements/wndoctavescript.cpp uiElements/wndoctavescript.ui
	    uiElements/wndradarmoduleeditor.h uiElements/wndradarmoduleeditor.cpp uiElements/wndradarmoduleeditor.ui
	    octaveInterface/octaveinterface.h octaveInterface/octaveinterface.cpp
	    radarModules/radarmodule.h radarModules/radarmodule.cpp

	    octaveInterface/octavews.h octaveInterface/octavews.cpp

	    radarModules/antenna.h radarModules/antenna.cpp
	    radarModules/radarinstance.h radarModules/radarinstance.cpp
	    radarModules/antennainstance.h radarModules/antennainstance.cpp
	    uiElements/octavesyntaxhighlighter.h uiElements/octavesyntaxhighlighter.cpp

	    uiElements/wnddefinecustomvar.h uiElements/wnddefinecustomvar.cpp uiElements/wnddefinecustomvar.ui

	    controller/radarcontroller.h controller/radarcontroller.cpp
	    projects/radarproject.h projects/radarproject.cpp
	    uiElements/wndantennadesigner.h uiElements/wndantennadesigner.cpp uiElements/wndantennadesigner.ui
	    octaveInterface/octavescript.h octaveInterface/octavescript.cpp
	    utils/ariautils.cpp
	    utils/ariautils.h
	    radarModules/radarparameter.h
	    radarModules/radarparameter.cpp
	    serialcomm/radarcommthread.h serialcomm/radarcommthread.cpp
	    uiElements/qdlgeditlimits.h uiElements/qdlgeditlimits.cpp uiElements/qdlgeditlimits.ui
	    uiElements/wnddirectionfocusing.h uiElements/wnddirectionfocusing.cpp uiElements/wnddirectionfocusing.ui
	    serialcomm/interfacecommand.h serialcomm/interfacecommand.cpp
	    serialcomm/interfaceprotocol.h serialcomm/interfaceprotocol.cpp
	    uiElements/wndradar.h uiElements/wndradar.cpp uiElements/wndradar.ui

	    uiElements/wndscanmodules.h uiElements/wndscanmodules.cpp uiElements/wndscanmodules.ui
	    utils/serialport_utils.h
	    utils/serialport_utils.cpp

	    uiElements/wndradarinstanceeditor.h uiElements/wndradarinstanceeditor.cpp uiElements/wndradarinstanceeditor.ui
	    uiElements/wndimportradar.h uiElements/wndimportradar.cpp uiElements/wndimportradar.ui

	    uiElements/wndplot2d.h uiElements/wndplot2d.cpp uiElements/wndplot2d.ui
	    uiElements/plotdescriptor.h
	    uiElements/wndradarliverun.h uiElements/wndradarliverun.cpp uiElements/wndradarliverun.ui
	    uiElements/plotdescriptor.cpp
	    uiElements/q_debugstream.h
	    scheduler/opscheduler.h scheduler/opscheduler.cpp
	    uiElements/wndscheduler.h uiElements/wndscheduler.cpp uiElements/wndscheduler.ui

	    uiElements/wndabout.h uiElements/wndabout.cpp uiElements/wndabout.ui
	    uiElements/wndfwupload.h uiElements/wndfwupload.cpp uiElements/wndfwupload.ui
        )
    endif()

    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

target_link_libraries(ARIA_RDK PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
#target_link_libraries(ARIA_RDK PRIVATE Qt${QT_VERSION_MAJOR}::DataVisualization)
target_link_libraries(ARIA_RDK PRIVATE Qt${QT_VERSION_MAJOR}::Sql)
target_link_libraries(ARIA_RDK PRIVATE Qt${QT_VERSION_MAJOR}::SerialPort)

#
# ARIA Sensing: customization to include Octave librariess
#

# Octave 9.2.0
target_link_libraries(ARIA_RDK PRIVATE "/usr/lib64/octave/9.2.0/liboctave.so")
target_link_libraries(ARIA_RDK PRIVATE "/usr/lib64/octave/9.2.0/liboctinterp.so")


# Armadillo
target_link_libraries(ARIA_RDK PRIVATE ${PROJECT_SOURCE_DIR}/../armadillo-12.6.7/package/lib64/libarmadillo.so)

# Graphic plot
target_link_libraries(ARIA_RDK PRIVATE JKQTPlotter${QT_VERSION_MAJOR}::JKQTPlotter${QT_VERSION_MAJOR})

#VTK
include(${VTK_USE_FILE})
target_link_libraries(ARIA_RDK PRIVATE ${VTK_LIBRARIES})

#Eigens
include_directories("/usr/local/include/eigen3/")

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.ss
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.ARIA_RDK)
endif()
set_target_properties(ARIA_RDK PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)

#
# ARIA Sensing: customization to include Octave librariess
#
# Octave 9.2,0

include_directories(AFTER "/usr/include/octave-9.2.0/octave")

# Armadillo
include_directories(ARIA_RDK PRIVATE ${PROJECT_SOURCE_DIR}/../armadillo-12.6.7/package/include/)
#SigPack
add_compile_definitions(HAVE_FFTW)
include_directories(ARIA_RDK PRIVATE ${PROJECT_SOURCE_DIR}/../sigpack-1.2.7/sigpack)
include_directories(AFTER ${PROJECT_SOURCE_DIR}/octaveInterface/)
include_directories(AFTER ${PROJECT_SOURCE_DIR}/controller/)
include_directories(AFTER ${PROJECT_SOURCE_DIR}/projects/)
include_directories(AFTER ${PROJECT_SOURCE_DIR}/radarModules)
include_directories(AFTER ${PROJECT_SOURCE_DIR}/serialcomm)
include_directories(AFTER ${PROJECT_SOURCE_DIR}/utils/)
include_directories(AFTER ${PROJECT_SOURCE_DIR}/uiElements/)
install(TARGETS ARIA_RDK
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")


if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(ARIA_RDK)
endif()


