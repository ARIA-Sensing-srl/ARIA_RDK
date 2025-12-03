cmake_minimum_required(VERSION 3.5)

project(ARIA_RDK VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


list(APPEND CMAKE_MODULE_PATH "~/Qt/6.9.3/gcc_64/lib/cmake/Qt6")

find_package(Qt6 REQUIRED COMPONENTS Widgets LinguistTools SerialPort Xml Sql)
find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets LinguistTools SerialPort Xml Sql)

set(TS_FILES ARIA_RDK_en_US.ts
  ARIA_RDK_en_US.ts)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
       ${TS_FILES}
)
#---------------------------------
# Windows
#---------------------------------

if (WIN32)
#---------------------------------
#paths
  set(JKQT_INSTALL_PREFIX "C:/msys64/usr/local/JKQtPlotter/")
  list(APPEND CMAKE_MODULE_PATH "${JKQT_INSTALL_PREFIX}/lib/cmake/JKQTPlotter6")
  list(APPEND CMAKE_PREFIX_PATH "${JKQT_INSTALL_PREFIX}/bin")
  list(PREPEND CMAKE_PREFIX_PATH "C:/msys64/mingw64/lib/cmake/vtk/patches/99")
  list(APPEND CMAKE_CURRENT_LIST_DIR "C:/msys64/usr/local/JKQTPlotter/lib/cmake/JKQTPlotter6")

#---------------------------------
#Qwt

  set(QWT_ROOT "C:/msys64/usr/local/qwt-6.3.0-dev")
  find_library( QWT NAMES qwt qwt6 PATHS ${QWT_ROOT}/lib REQUIRED )
  set(ENV{QT_PLUGIN_PATH} "${QWT_ROOT}/plugins:$QT_PLUGIN_PATH")
  message(QWT_ROOT="${QWT_ROOT}")
  include_directories(AFTER "${QWT_ROOT}/include")

#---------------------------------
# VTK
  #set(VTK_DIR "C:/msys64/home/Alessio/ARIA_RDK/VTK/lib/cmake/vtk-9.4")

#---------------------------------
# QScintilla

find_library( QSCI_LIBRARY_PATH NAME qscintilla2_qt6 PATHS "/usr/lib"  REQUIRED )
include_directories(AFTER "${QSCI_LIBRARY_PATH}/include")
get_filename_component(QSCINTILLA_LIB_PATH "${QSCI_LIBRARY_PATH}" DIRECTORY)
cmake_path(GET QSCINTILLA_LIB_PATH PARENT_PATH QSCINTILLA_PARENT)
set(QSCINTILLA_INSTALL_PATH "${QSCINTILLA_PARENT}/bin")
message("QSCINTILLA_INSTALL_PATH set to ${QSCINTILLA_INSTALL_PATH}")
#---------------------------------
#Octave
set(OCTAVE_VERSION "10.2.0")
set(OCTAVE_INSTALL_PATH "C:/msys64/mingw64/")
include_directories(AFTER "${OCTAVE_INSTALL_PATH}/include/octave-${OCTAVE_VERSION}/octave")

#---------------------------------
# Eigen
  set(EIGEN_ROOT "C:/A/eigen-3.4.0")

#---------------------------------
# Link directories

  link_directories(
                    "${JKQT_INSTALL_PREFIX}/bin"
                    "${QSCINTILLA_INSTALL_PATH}"
                    "${QSCINTILLA_LIB_PATH}"
                    "${QWT_ROOT}/lib"
                    )

#---------------------------------
#End of Windows
#---------------------------------
else()
#---------------------------------
# Linux
#---------------------------------
  set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/${CMAKE_BUILD_TYPE}")

#Qwt

  set(QWT_ROOT "/usr/local/qwt-6.3.0/")
  find_path( QWT_INCLUDE NAME qwt.h PATHS ${QWT_ROOT}/include REQUIRED )
  cmake_path(GET QWT_INCLUDE PARENT_PATH ${QWT_ROOT})
  include_directories(${QWT_INCLUDE})


#---------------------------------
# Octave
   set(OCTAVE_INSTALL_PATH "~/my_octave")
   set(OCTAVE_VERSION "10.3.0")
   find_library( OCTAVE_LIBRARY      NAME octinterp PATHS ${OCTAVE_INSTALL_PATH}/lib/octave/${OCTAVE_VERSION} REQUIRED )
   cmake_path(GET OCTAVE_LIBRARY PARENT_PATH OCTAVE_LIBRARY_PATH)
   find_path (    OCTAVE_INCLUDE_PATH NAME octave.h  PATHS ${OCTAVE_INSTALL_PATH}/include/octave-${OCTAVE_VERSION}/octave REQUIRED )
   include_directories( AFTER ${OCTAVE_INCLUDE_PATH} )

#---------------------------------
# Eigen


  set(EIGEN_ROOT "/usr/local/include/eigen3/Eigen")
  find_path ( EIGEN_INCLUDE_PATH NAME Eigen PATHS ${EIGEN_ROOT} REQUIRED )
  cmake_path( GET  EIGEN_INCLUDE_PATH PARENT_PATH EIGEN_ROOT)
  include_directories(AFTER ${EIGEN_INCLUDE_PATH})
  include_directories(AFTER ${EIGEN_INCLUDE_PATH}/../)

#---------------------------------
# QScintilla

  find_library( QSCI_LIBRARY_PATH NAME qscintilla2_qt6 PATHS "/usr/lib"  REQUIRED )
  cmake_path(GET QSCI_LIBRARY_PATH PARENT_PATH QSCINTILLA_INSTALL_PATH)
  message("Qsci library path ${QSCINTILLA_INSTALL_PATH}")

#---------------------------------
# JKQT
  set(JKQT_INSTALL_PREFIX "/usr/local/jkqtplotter/")
  list(APPEND CMAKE_PREFIX_PATH "${JKQT_INSTALL_PREFIX}/lib64/cmake/JKQTPlotter6")

#---------------------------------
#End of Linux
#---------------------------------

endif()

#---------------------------------
#---------------------------------
# Common
#---------------------------------
#Qwt

  find_library( QWT NAMES qwt qwt6 PATHS ${QWT_ROOT}/lib REQUIRED )
  set(ENV{QT_PLUGIN_PATH} "${QWT_ROOT}/plugins:$QT_PLUGIN_PATH")
  message(QWT_ROOT="${QT_PLUGIN_PATH}")
  include_directories(AFTER "${QT_PLUGIN_PATH}/include")

#---------------------------------
# VTK
  find_package(VTK REQUIRED)
  include(${VTK_USE_FILE})

#---------------------------------
# JKQTPlotter
  find_package(JKQTPlotter${QT_VERSION_MAJOR} REQUIRED)
  message("JKQT Root set at ${JKQT_INSTALL_PREFIX}")
#---------------------------------
# Eigen

  include_directories("${EIGEN_ROOT}")
#---------------------------------

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
        octaveInterface/aria_rdk_interface_messages.h
        octaveInterface/aria_rdk_interface_message.cpp
        uiElements/dlgqwtplot.h uiElements/dlgqwtplot.cpp uiElements/dlgqwtplot.ui
        uiElements/plotdata.h uiElements/plotdata.cpp

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

if (WIN32)
  target_compile_definitions(ARIA_RDK PRIVATE x64)
endif()


target_link_libraries(ARIA_RDK PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(ARIA_RDK PRIVATE Qt${QT_VERSION_MAJOR}::Xml)
target_link_libraries(ARIA_RDK PRIVATE Qt${QT_VERSION_MAJOR}::SerialPort)
target_link_libraries(ARIA_RDK PRIVATE Qt${QT_VERSION_MAJOR}::Sql)

target_link_libraries(ARIA_RDK PRIVATE ${QWT} )
target_link_libraries(ARIA_RDK PRIVATE JKQTPlotter${QT_VERSION_MAJOR}::JKQTPlotter${QT_VERSION_MAJOR})
target_link_libraries(ARIA_RDK PRIVATE ${VTK_LIBRARIES})


if (WIN32)

  target_link_libraries(ARIA_RDK PRIVATE "${OCTAVE_INSTALL_PATH}/lib/octave/${OCTAVE_VERSION}/liboctave.dll.a")
  target_link_libraries(ARIA_RDK PRIVATE "${OCTAVE_INSTALL_PATH}/lib/octave/${OCTAVE_VERSION}/liboctinterp.dll.a")
  target_link_libraries(ARIA_RDK PRIVATE "${QSCINTILLA_INSTALL_PATH}/../bin/qscintilla2_qt6.dll")
  target_link_libraries(ARIA_RDK PRIVATE "${QWT_ROOT}/lib/libqwt.dll.a")

else()

  message("Octave path ${OCTAVE_LIBRARY_PATH}")
  target_link_libraries(ARIA_RDK PRIVATE "${OCTAVE_LIBRARY_PATH}/liboctave.so")
  target_link_libraries(ARIA_RDK PRIVATE "${OCTAVE_LIBRARY_PATH}/liboctinterp.so")
  target_link_libraries(ARIA_RDK PRIVATE "${QSCINTILLA_INSTALL_PATH}/libqscintilla2_qt6.so")

endif()

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

if (WIN32)
  #SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath -Wl,"C:/Program Files/GNU Octave/Octave-9.4.0/mingw64/bin"")
else()
  include(GNUInstallDirs)
endif()


include_directories(AFTER ${PROJECT_SOURCE_DIR}/octaveInterface/)
include_directories(AFTER ${PROJECT_SOURCE_DIR}/controller/)
include_directories(AFTER ${PROJECT_SOURCE_DIR}/projects/)
include_directories(AFTER ${PROJECT_SOURCE_DIR}/radarModules)
include_directories(AFTER ${PROJECT_SOURCE_DIR}/serialcomm)
include_directories(AFTER ${PROJECT_SOURCE_DIR}/utils/)
include_directories(AFTER ${PROJECT_SOURCE_DIR}/editor/)
include_directories(AFTER ${PROJECT_SOURCE_DIR}/uiElements/)

message("${CMAKE_INSTALL_BINDIR} ; ${CMAKE_INSTALL_LIBDIR}")



SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pthread")
SET(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR}/output/${CMAKE_BUILD_TYPE})

message("Output folder : ${CMAKE_INSTALL_PREFIX}")
message("Binary folder : ${CMAKE_INSTALL_BINDIR}")
#-------------------------------------------------
# Deploy Qt and all binaries (dlls)

# --- windeployqt integration ---
if(WIN32)

#-------------------------------------------------------
# Deploy JKQT
        install(
                        DIRECTORY "${JKQT_INSTALL_PREFIX}/bin/"
                        DESTINATION "${CMAKE_INSTALL_BINDIR}"
                        FILES_MATCHING PATTERN "*.dll"
                        )
#-------------------------------------------------------
# Deploy QWT
        install(
                        DIRECTORY "${QWT_ROOT}/lib/"
                        DESTINATION "${CMAKE_INSTALL_BINDIR}"
                        FILES_MATCHING PATTERN "*.dll"
                        )
#-------------------------------------------------------
# Deploy Octave & QScintilla
        set(MSYS_MINGW_HOME "C:/msys64/mingw64/")

        install(
                        DIRECTORY "${MSYS_MINGW_HOME}/bin/"
                        DESTINATION "${CMAKE_INSTALL_BINDIR}"
                        FILES_MATCHING PATTERN "*.dll"
                        )

        install(
                        DIRECTORY "${MSYS_MINGW_HOME}/../usr/bin/"
                        DESTINATION "${CMAKE_INSTALL_BINDIR}"
                        FILES_MATCHING PATTERN "*.dll"
                        )

#-------------------------------------------------------
# Deploy Required binaries for octave & toolboxes
                install(
                        DIRECTORY "${MSYS_MINGW_HOME}/include/"
                        DESTINATION "${CMAKE_INSTALL_PREFIX}/include/"
                        FILES_MATCHING PATTERN "*"
                        )
                install(
                        DIRECTORY "${MSYS_MINGW_HOME}/bin/"
                        DESTINATION "${CMAKE_INSTALL_BINDIR}"
                        FILES_MATCHING PATTERN "*.exe"
                        )
        install(
                        DIRECTORY "${MSYS_MINGW_HOME}/lib/"
                        DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/"
                        FILES_MATCHING PATTERN "*"
                        )
        install(
                        DIRECTORY "${MSYS_MINGW_HOME}/../usr/bin/"
                        DESTINATION "${CMAKE_INSTALL_PREFIX}/../usr/bin/"
                        FILES_MATCHING PATTERN "sh.exe"
                        )
        install(
                        DIRECTORY "${MSYS_MINGW_HOME}/../usr/bin/"
                        DESTINATION "${CMAKE_INSTALL_BINDIR}"
                        FILES_MATCHING PATTERN "*.exe"
                        )
        install(
                        DIRECTORY "${MSYS_MINGW_HOME}/share/octave/"
                        DESTINATION "${CMAKE_INSTALL_PREFIX}/share/octave/"
                        FILES_MATCHING PATTERN "*"
                        )

        install(
                        DIRECTORY "${PROJECT_SOURCE_DIR}/octave/toolboxes/"
                        DESTINATION "${CMAKE_INSTALL_PREFIX}/octave/toolboxes/"
                        FILES_MATCHING PATTERN "*.zip"
                        )

#-------------------------------------------------------
# Finally Deploy Qt and all binaries (dlls)
                install(TARGETS ARIA_RDK
                        BUNDLE DESTINATION .
                        LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR}
                        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                )

                qt_generate_deploy_qml_app_script(
                        TARGET ARIA_RDK
                        OUTPUT_SCRIPT deploy_script
                )
                install(SCRIPT ${deploy_script})

#-------------------------------------------------------
# Deploy modified m files  for toolboxes (both Windows)
# In Windows, we need to override the unpack.m due to issues with /C/, C:

        install(DIRECTORY "${PROJECT_SOURCE_DIR}/octave/${OCTAVE_VERSION}/m/"
                        DESTINATION "${CMAKE_INSTALL_PREFIX}/share/octave/${OCTAVE_VERSION}/m/"
                        FILES_MATCHING PATTERN "*.m")

#-------------------------------------------------------
# Deploy modules definitions!

        else()
# Linux
        message("Deploying Qt into ${CMAKE_INSTALL_PREFIX}")

        install(TARGETS ARIA_RDK
            BUNDLE DESTINATION .
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        )

        install(
                        DIRECTORY "${PROJECT_SOURCE_DIR}/octave/toolboxes/"
                        DESTINATION "${CMAKE_INSTALL_PREFIX}/octave/toolboxes/"
                        FILES_MATCHING PATTERN "*.zip"
                        )

        install(
                        DIRECTORY "${OCTAVE_INSTALL_PATH}/share/octave/"
                        DESTINATION "${CMAKE_INSTALL_PREFIX}/share/octave/"
                        FILES_MATCHING PATTERN "*"
                        )
endif()


if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(ARIA_RDK)
endif()
