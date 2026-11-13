cmake_minimum_required(VERSION 3.16)
project(VxVoice
    VERSION 1.4.9
    LANGUAGES CXX
    DESCRIPTION "VxVoice - Enterprise SIP Softphone Client (PJSIP Backend)"
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DVXVOICE_VERSION="${PROJECT_VERSION}")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Network
)
find_package(Qt6 OPTIONAL_COMPONENTS Multimedia)

# Arquivos de código-fonte
set(SOURCES
    src/main.cpp
    src/core/SipManager.cpp
    src/core/SettingsManager.cpp
    src/core/ContactsManager.cpp
    src/core/ThemeManager.cpp
    src/core/SoundManager.cpp
    src/core/UpdateManager.cpp
    src/ui/MainWindow.cpp
    src/ui/CallWidget.cpp
    src/ui/HistoryWidget.cpp
    src/ui/ContactsWidget.cpp
    src/ui/RecordingsWidget.cpp
    src/ui/ActiveCallDialog.cpp
    src/ui/SettingsDialog.cpp
    src/ui/AccountsDialog.cpp
    src/ui/CallNotificationPopup.cpp
    src/ui/SelectContactNumberDialog.cpp
    src/ui/ChangelogDialog.cpp
    src/ui/UpdateProgressDialog.cpp
    src/ui/IconHelper.cpp
)

set(HEADERS
    src/core/SipManager.h
    src/core/SettingsManager.h
    src/core/ContactsManager.h
    src/core/ThemeManager.h
    src/core/SoundManager.h
    src/core/UpdateManager.h
    src/ui/MainWindow.h
    src/ui/CallWidget.h
    src/ui/HistoryWidget.h
    src/ui/ContactsWidget.h
    src/ui/RecordingsWidget.h
    src/ui/ActiveCallDialog.h
    src/ui/SettingsDialog.h
    src/ui/AccountsDialog.h
    src/ui/CallNotificationPopup.h
    src/ui/SelectContactNumberDialog.h
    src/ui/ChangelogDialog.h
    src/ui/UpdateProgressDialog.h
    src/ui/IconHelper.h
)

set(RESOURCES
    resources/resources.qrc
)

if(WIN32)
    list(APPEND SOURCES resources/app.rc)
endif()

add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
)

if(TARGET Qt6::Multimedia)
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Multimedia)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_QT_MULTIMEDIA)
endif()

# Suporte ao motor físico PJSIP / PJSUA2
find_package(PkgConfig REQUIRED)
pkg_check_modules(PJPROJECT REQUIRED libpjproject)
target_include_directories(${PROJECT_NAME} PRIVATE ${PJPROJECT_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${PJPROJECT_LIBRARIES})
target_compile_definitions(${PROJECT_NAME} PRIVATE USE_REAL_PJSIP)

if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# ==============================================================================
# Diretivas de Instalação (FHS Linux & Estrutura Windows)
# ==============================================================================
include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(UNIX AND NOT APPLE)
    # Instalação do arquivo de atalho desktop corporativo
    install(FILES resources/vxvoice.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
    )
    # Instalação do ícone oficial nos diretórios de ícones do Linux (múltiplas resoluções)
    foreach(res 16 24 32 48 64 128 256)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/${res}x${res}/vxvoice.png")
            install(FILES "resources/icons/${res}x${res}/vxvoice.png"
                DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/${res}x${res}/apps"
            )
        endif()
    endforeach()
    install(FILES resources/icons/app_icon.png
        DESTINATION ${CMAKE_INSTALL_DATADIR}/pixmaps
        RENAME vxvoice.png
    )
endif()

# ==============================================================================
# CPack Packaging Configuration (Multiplataforma CI/CD: .msi via WiX e .deb)
# ==============================================================================
set(CPACK_PACKAGE_NAME "vxvoice")
set(CPACK_PACKAGE_VENDOR "VxNET Comunicações e Telecomunicações")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "VxVoice - Enterprise SIP Softphone Client")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_CONTACT "suporte@vxnet.com.br")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://www.vxnet.com.br")

# Linux Packaging: Pacote Debian (.deb) e Tarball autocontido
if(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "DEB;TGZ")
    
    set(CPACK_DEBIAN_PACKAGE_NAME "vxvoice")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "VxNET Telecom <suporte@vxnet.com.br>")
    set(CPACK_DEBIAN_PACKAGE_SECTION "net")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS OFF) # Definido explícito para compatibilidade ampla
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.31), libqt6widgets6t64 | libqt6widgets6, libqt6network6t64 | libqt6network6, libqt6gui6t64 | libqt6gui6, libqt6core6t64 | libqt6core6")
    set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "VxVoice Softphone Corporativo SIP de alta fidelidade desenvolvido em C++ e Qt6 com backend PJSIP.")
    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/debian/prerm"
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/debian/postinst"
    )
endif()

# Windows Packaging: WiX Toolset gerando instalador corporativo .msi para GPO
if(WIN32)
    set(CPACK_GENERATOR "WIX;ZIP")
    
    # GUIDs fixos para garantir Upgrade transparente em políticas corporativas GPO
    set(CPACK_WIX_UPGRADE_GUID "A5D19302-7B4F-4E89-9A52-87F95A34DC7A")
    set(CPACK_WIX_PRODUCT_GUID "B6E29413-8C50-5F9A-0B63-980A6B45ED8B")
    
    set(CPACK_WIX_PRODUCT_NAME "VxVoice Enterprise Softphone")
    set(CPACK_WIX_PACKAGE_NAME "VxVoice")
    set(CPACK_WIX_PROGRAM_MENU_FOLDER "VxNET Telecom\\VxVoice")
    set(CPACK_WIX_UI_REF "WixUI_InstallDir")
    set(CPACK_WIX_INSTALL_SCOPE "perMachine")
    set(CPACK_WIX_CREATE_DESKTOP_SHORTCUT ON)
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "VxVoice")
endif()

include(CPack)
