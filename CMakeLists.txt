# SprintIDE/CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(SprintIDE VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 自动处理Qt的moc、uic、rcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 查找Qt组件
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Network Concurrent LinguistTools Svg)

# 应用程序信息
set(APP_NAME "SprintIDE")
set(APP_VERSION "1.0.0")

# ========== 按照设计文档的模块结构组织 ==========
# Core Framework (核心框架)
set(CORE_SOURCES
    application.cpp
    config_manager.cpp
    theme_manager.cpp
    font_manager.cpp
)

set(CORE_HEADERS
    application.h
    config_manager.h
    theme_manager.h
    font_manager.h
)

# UI Module (界面模块)
set(UI_SOURCES
    mainwindow.cpp
    editor_widget.cpp
    tab_widget.cpp
    editor_settings_page.cpp
)

set(UI_HEADERS
    mainwindow.h
    mainwindow_p.h
    editor_widget.h
    tab_widget.h
    editor_settings_page.h
)

set(UI_FORMS
    mainwindow.ui
)

# Editor Module (编辑器模块)
set(EDITOR_SOURCES
    text_editor.cpp
    syntax_highlighter.cpp
)

set(EDITOR_HEADERS
    text_editor.h
    syntax_highlighter.h
)

# Project Management (项目管理模块)
set(PROJECT_MANAGEMENT_SOURCES
    project_config.cpp
    project_creation_dialog.cpp
    project_panel.cpp
    project_manager.cpp
    project_selection_dialog.cpp
)

set(PROJECT_MANAGEMENT_HEADERS
    project_config.h
    project_creation_dialog.h
    project_panel.h
    project_manager.h
    project_selection_dialog.h
)

set(PROJECT_MANAGEMENT_FORMS
    project_creation_dialog.ui
)

set(UI_DIALOG_SOURCES
    find_dialog.cpp
    replace_dialog.cpp
)

set(UI_DIALOG_HEADERS
    find_dialog.h
    replace_dialog.h
)
# 添加到 ALL_SOURCES
# set(ALL_SOURCES
#     ${ALL_SOURCES}
#     ${UI_DIALOG_SOURCES}
# )

# set(ALL_HEADERS
#     ${ALL_HEADERS}
#     ${UI_DIALOG_HEADERS}
#     ${ALL_HEADERS}
#     ${LSP_HEADERS}
# )

# Compile System (编译系统模块 - 占位)
# set(COMPILE_SYSTEM_SOURCES
    # compile_manager.cpp - 稍后添加
    # compile_output_parser.cpp - 稍后添加
    # bottom_panel.cpp - 稍后添加
# )

# set(COMPILE_SYSTEM_HEADERS
    # compile_manager.h - 稍后添加
    # compile_output_parser.h - 稍后添加
    # bottom_panel.h - 稍后添加
# )

# 资源文件
set(RESOURCE_FILES
    resources/lightide_resources.qrc
    resources/fonts.qrc
)

# 检查资源文件是否存在
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/lightide_resources.qrc")
    message(STATUS "找到资源文件: resources/lightide_resources.qrc")
else()
    message(WARNING "资源文件不存在: resources/lightide_resources.qrc")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/fonts.qrc")
    message(STATUS "找到字体资源文件: resources/fonts.qrc")
else()
    message(WARNING "字体资源文件不存在: resources/fonts.qrc - 请手动下载JetBrains Mono字体")
endif()

# 所有源文件
set(ALL_SOURCES
    main.cpp
    ${CORE_SOURCES}
    ${UI_SOURCES}
    ${EDITOR_SOURCES}
    ${PROJECT_MANAGEMENT_SOURCES}
    # ${COMPILE_SYSTEM_SOURCES}
    ${RESOURCE_FILES}
    ${ALL_SOURCES}
    ${LSP_SOURCES}
    ${UI_DIALOG_SOURCES}
    ${TERMINAL_SOURCES}
    ${AI_SOURCES}
    ${LANGUAGE_SOURCES}
    ${HISTORY_SOURCES}
    ${BUILDER_SOURCES}
    ${UTILS_SOURCES}
    ${JAVA_SOURCES}
    ${PYTHON_SOURCES}
    ${CPP_SOURCES}
    ${DEPS_SOURCES}
    ${DAP_SOURCES}
    ${GIT_SOURCES}
    ${COMPLETION_SOURCES}
)

set(ALL_HEADERS
    ${CORE_HEADERS}
    ${UI_HEADERS}
    ${EDITOR_HEADERS}
    ${PROJECT_MANAGEMENT_HEADERS}
    # ${COMPILE_SYSTEM_HEADERS}
    ${ALL_HEADERS}
    ${LSP_HEADERS}
    ${UI_DIALOG_HEADERS}
    ${TERMINAL_HEADERS}
    ${AI_HEADERS}
    ${LANGUAGE_HEADERS}
    ${HISTORY_HEADERS}
    ${BUILDER_HEADERS}
    ${UTILS_HEADERS}
    ${JAVA_HEADERS}
    ${PYTHON_HEADERS}
    ${CPP_HEADERS}
    ${DEPS_HEADERS}
    ${DAP_HEADERS}
    ${GIT_HEADERS}
    ${COMPLETION_HEADERS}
)

set(ALL_FORMS
    ${UI_FORMS}
    ${PROJECT_MANAGEMENT_FORMS}
)

# LSP Client 模块
set(LSP_SOURCES
    src/lsp/lsp_client.cpp
    src/lsp/lsp_manager.cpp
    src/lsp/diagnostic_filter.cpp
)

set(LSP_HEADERS
    src/lsp/lsp_client.h
    src/lsp/lsp_manager.h
    src/lsp/diagnostic_filter.h
)

# TERMINA  模块
set(TERMINAL_SOURCES
    src/terminal/terminal_panel.cpp
)

set(TERMINAL_HEADERS
    src/terminal/terminal_panel.h
)

set(AI_SOURCES
    src/ai/ai_server.cpp
    src/ai/ai_action_executor.cpp
    src/ai/chat_panel.cpp
    src/ai/ai_integration.cpp
    ai/ai_settings_page.cpp
    ai/transport/tcp_transport.cpp
    ai/ai_connection_manager.cpp
)

set(AI_HEADERS
    src/ai/ai_server.h
    src/ai/ai_action_executor.h
    src/ai/chat_panel.h
    src/ai/ai_integration.h
    ai/ai_settings_page.h
    ai/transport/ai_transport.h
    ai/transport/tcp_transport.h
    ai/ai_connection_manager.h
)

set(LANGUAGE_SOURCES
    src/language/language_detector.cpp
    src/language/language_highlighter.cpp
    src/language/editor_rules.cpp
)

set(LANGUAGE_HEADERS
    src/language/language_detector.h
    src/language/language_highlighter.h
    src/language/language_type.h
    src/language/editor_rules.h

)

set(HISTORY_HEADERS
    src/history/local_history.h
)

set(HISTORY_SOURCES
    src/history/local_history.cpp
)

set(BUILDER_SOURCES
    src/builder/cpp_builder.cpp
    src/builder/python_builder.cpp
    src/builder/java_gradle_builder.cpp
    src/builder/go_builder.cpp
    src/builder/rust_builder.cpp
    src/builder/node_builder.cpp
    src/builder/generic_builder.cpp
    src/builder/builder_factory.cpp
    src/builder/java_maven_builder.cpp
)

set(BUILDER_HEADERS
    src/builder/build_task.h
    src/builder/build_result.h
    src/builder/ibuilder.h
    src/builder/cpp_builder.h
    src/builder/python_builder.h
    src/builder/java_gradle_builder.h
    src/builder/go_builder.h
    src/builder/rust_builder.h
    src/builder/node_builder.h
    src/builder/generic_builder.h
    src/builder/builder_factory.h
    src/builder/java_maven_builder.h
)

set(UTILS_SOURCES
    src/utils/java_util.cpp
)

set(UTILS_HEADERS
    src/utils/java_util.h
)

set(JAVA_SOURCES
    src/java/project_model.cpp
    src/java/maven_project_model.cpp
    src/java/gradle_project_model.cpp
    src/java/plain_java_project_model.cpp
    src/java/project_resolver.cpp
    src/run/run_configuration.cpp
    src/run/run_config_manager.cpp
    src/run/java_launcher.cpp
)

set(JAVA_HEADERS
    src/java/project_model.h
    src/java/maven_project_model.h
    src/java/gradle_project_model.h
    src/java/plain_java_project_model.h
    src/java/project_resolver.h
    src/run/run_configuration.h
    src/run/run_config_manager.h
    src/run/java_launcher.h
)

set(PYTHON_SOURCES
    src/python/python_virtual_env.cpp
    src/python/python_project_model.cpp
    src/python/python_dependency_manager.cpp
    src/python/python_run_config.cpp
    src/python/python_debugger.cpp
    src/python/python_formatter.cpp
    src/python/python_test_runner.cpp
    src/python/python_settings_page.cpp
    src/python/python_run_config_widget.cpp
)
set(PYTHON_HEADERS
    src/python/python_virtual_env.h
    src/python/python_project_model.h
    src/python/python_dependency_manager.h
    src/python/python_run_config.h
    src/python/python_debugger.h
    src/python/python_formatter.h
    src/python/python_test_runner.h
    src/python/python_settings_page.h
    src/python/python_run_config_widget.h
)

set(CPP_SOURCES
    src/cpp/cpp_project_model.cpp
)

set(CPP_HEADERS
    src/cpp/cpp_project_model.h
)

set(DEPS_SOURCES
    src/deps/toolchain_manager.cpp
    src/deps/lldb_dap_provider.cpp
    src/deps/toolchain_dialog.cpp
    src/deps/gdb_dap_provider.cpp
)

set(DEPS_HEADERS
    src/deps/toolchain_types.h
    src/deps/toolchain_provider.h
    src/deps/toolchain_manager.h
    src/deps/lldb_dap_provider.h
    src/deps/toolchain_dialog.h
    src/deps/gdb_dap_provider.h
)

set(DAP_SOURCES
    src/dap/dap_client.cpp
)

set(DAP_HEADERS
    src/dap/dap_client.h
    src/dap/dap_types.h
)

set(GIT_SOURCES
    src/git/git_command.cpp
    src/git/git_manager.cpp
    src/git/git_panel.cpp
    src/git/git_overlay_proxy_model.cpp
    src/git/git_settings_page.cpp
    src/git/git_diff_dialog.cpp
    src/git/git_file_history_dialog.cpp
)

set(GIT_HEADERS
    src/git/git_types.h
    src/git/git_command.h
    src/git/git_manager.h
    src/git/git_panel.h
    src/git/git_overlay_proxy_model.h
    src/git/git_settings_page.h
    src/git/git_diff_dialog.h
    src/git/git_file_history_dialog.h
)

set(CORE_SOURCES
    core/log_buffer.cpp
)

set(CORE_HEADERS
    core/log_buffer.h
)

set(COMPLETION_SOURCES
    editor/completion_model.cpp
    editor/completion_popup.cpp
    editor/completion_delegate.cpp
)

set(COMPLETION_HEADERS
    editor/completion_model.h
    editor/completion_popup.h
    editor/completion_delegate.h
)

# 创建可执行文件
add_executable(${PROJECT_NAME}
    ${ALL_SOURCES}
    ${ALL_HEADERS}
    ${ALL_FORMS}
    process_manager.h
    path_utils.h
    compile_manager.h
    process_manager.cpp
    path_utils.cpp
    compile_manager.cpp
    compile_output_parser.h
    compile_output_parser.cpp
    debug_manager.h
    debug_manager.cpp
    debug_panel.h
    debug_panel.cpp
    find_dialog.h
    find_dialog.cpp
    replace_dialog.h
    replace_dialog.cpp
    lsp/lsp_client.h
    lsp/lsp_client.cpp
    terminal/terminal_panel.h
    terminal/terminal_panel.cpp
    ai/ai_server.h
    ai/ai_server.cpp
    ai/ai_action_executor.h
    ai/ai_action_executor.cpp
    ai/chat_panel.h
    ai/chat_panel.cpp
    ai/ai_integration.cpp
    ai/ai_integration.h
    ai/transport/ai_transport.h
    ai/transport/tcp_transport.h
    ai/transport/tcp_transport.cpp
    ai/ai_connection_manager.h
    ai/ai_connection_manager.cpp
    language/language_detector.h
    language/language_detector.cpp
    language/language_highlighter.h
    language/language_highlighter.cpp
    lsp/lsp_manager.h
    lsp/lsp_manager.cpp
    history/local_history.h
    history/local_history.cpp
    language/language_type.h
    builder/build_task.h
    builder/build_result.h
    builder/ibuilder.h
    builder/cpp_builder.h
    builder/cpp_builder.cpp
    builder/python_builder.h
    builder/python_builder.cpp
    builder/generic_builder.h
    builder/generic_builder.cpp
    builder/builder_factory.h
    builder/builder_factory.cpp
    builder/java_gradle_builder.h
    builder/java_gradle_builder.cpp
    builder/go_builder.h
    builder/go_builder.cpp
    builder/rust_builder.h
    builder/rust_builder.cpp
    builder/node_builder.h
    builder/node_builder.cpp
    settings_dialog.h
    settings_dialog.cpp
    builder/java_maven_builder.h
    builder/java_maven_builder.cpp
    builder/java_builder.h
    builder/java_builder.cpp
    utils/java_utils.h
    utils/java_utils.cpp
    java/project_model.h
    java/project_model.cpp
    java/maven_project_model.h
    java/maven_project_model.cpp
    java/gradle_project_model.h
    java/gradle_project_model.cpp
    java/plain_java_project_model.h
    java/plain_java_project_model.cpp
    java/project_resolver.h
    java/project_resolver.cpp
    run/run_configuration.h
    run/run_config_manager.h
    run/run_config_manager.cpp
    run/java_launcher.h
    run/java_launcher.cpp
    run/run_configuration.cpp
    python/python_project_model.h
    python/python_virtual_env.h
    python/python_virtual_env.cpp
    python/python_project_model.cpp
    python/python_dependency_manager.h
    python/python_dependency_manager.cpp
    python/python_run_config.h
    python/python_run_config.cpp
    python/python_debugger.h
    python/python_debugger.cpp
    python/python_formatter.h
    python/python_formatter.cpp
    python/python_test_runner.h
    python/python_test_runner.cpp
    python/python_settings_page.h
    python/python_settings_page.cpp
    python/python_run_config_widget.h
    python/python_run_config_widget.cpp
    cpp/cpp_project_model.h
    cpp/cpp_project_model.cpp
    deps/toolchain_manager.h
    deps/toolchain_manager.cpp
    deps/toolchain_provider.h
    deps/lldb_dap_provider.h deps/lldb_dap_provider.cpp
    deps/toolchain_types.h
    deps/toolchain_dialog.h
    deps/toolchain_dialog.cpp
    dap/dap_types.h
    dap/dap_client.h
    dap/dap_client.cpp
    cpp/cpp_debugger.h
    cpp/cpp_debugger.cpp
    editor_settings_page.h
    editor_settings_page.cpp
    deps/gdb_dap_provider.h
    deps/gdb_dap_provider.cpp
    ai/ai_settings_page.h
    ai/ai_settings_page.cpp
    git/git_types.h
    git/git_command.h
    git/git_command.cpp
    git/git_manager.h
    git/git_manager.cpp
    git/git_panel.h
    git/git_panel.cpp
    git/git_overlay_proxy_model.h
    git/git_overlay_proxy_model.cpp
    git/git_settings_page.h git/git_settings_page.cpp
    git/git_diff_dialog.h git/git_diff_dialog.cpp
    git/git_file_history_dialog.h
    git/git_file_history_dialog.cpp
    core/log_buffer.h
    core/log_buffer.cpp
    language/editor_rules.h
    language/editor_rules.cpp
    editor/completion_model.h
    editor/completion_model.cpp
    editor/completion_popup.h
    editor/completion_popup.cpp
    editor/completion_delegate.h
    editor/completion_delegate.cpp
    lsp/diagnostic_filter.h
    lsp/diagnostic_filter.cpp
    quick_open_dialog.h
    quick_open_dialog.cpp
    performance/performance_monitor.h
    performance/performance_monitor.cpp
    breakpoint_list_panel.h
    breakpoint_list_panel.cpp
    search_in_files_dialog.h search_in_files_dialog.cpp

    log/log_list_model.h log/log_list_model.cpp
    log/log_list_view.h log/log_list_view.cpp
    log/log_item_delegate.h log/log_item_delegate.cpp
    log/output_panel.h log/output_panel.cpp
    translation/language_manager.h translation/language_manager.cpp
)

# ============================================================
# 国际化（i18n）：.ts → .qm 并打包进 Qt 资源系统
#   - 运行时通过 ":/i18n/SprintIDE_<locale>.qm" 加载
#   - 若 translations/ 目录下没有 .ts，则本段自动跳过（首次 clone 不报错）
# ============================================================
set(SPRINTIDE_TS_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/translations/SprintIDE_zh_CN.ts"
    "${CMAKE_CURRENT_SOURCE_DIR}/translations/SprintIDE_en_US.ts"
)

set(SPRINTIDE_EXISTING_TS)
foreach(_ts ${SPRINTIDE_TS_FILES})
    if(EXISTS "${_ts}")
        list(APPEND SPRINTIDE_EXISTING_TS "${_ts}")
    else()
        message(STATUS "[i18n] 翻译文件暂不存在（跳过）: ${_ts}")
    endif()
endforeach()

if(SPRINTIDE_EXISTING_TS)
    qt_add_translations(${PROJECT_NAME}
        TS_FILES       ${SPRINTIDE_EXISTING_TS}
        RESOURCE_PREFIX "/i18n"
    )
    list(LENGTH SPRINTIDE_EXISTING_TS _ts_count)
    message(STATUS "[i18n] 已启用，翻译文件数: ${_ts_count}")
else()
    message(STATUS "[i18n] 未启用（translations/ 目录下没有 .ts）")
endif()

# 链接Qt库
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::Network
    Qt6::Concurrent
    Qt6::Svg
)

# ========== 使用source_group创建设计文档中的模块结构 ==========

# Main Entry (程序入口)
source_group("Main" FILES main.cpp)

# Core Framework (核心框架)
source_group("Core Framework" FILES
    ${CORE_SOURCES}
    ${CORE_HEADERS}
)

source_group("Core Framework\\Application" FILES
    application.cpp application.h)
source_group("Core Framework\\Config" FILES
    config_manager.cpp config_manager.h)
source_group("Core Framework\\Theme" FILES
    theme_manager.cpp theme_manager.h)
source_group("Core Framework\\Font" FILES
    font_manager.cpp font_manager.h)

# UI Module (界面模块)
source_group("UI" FILES
    ${UI_SOURCES}
    ${UI_HEADERS}
    ${UI_FORMS}
)

source_group("UI\\MainWindow" FILES
    mainwindow.cpp mainwindow.h mainwindow.ui
    mainwindow_p.h
    tab_widget.cpp tab_widget.h
)

source_group("UI\\Editor" FILES
    editor_widget.cpp editor_widget.h
)

# Editor Module (编辑器模块)
source_group("Editor" FILES
    ${EDITOR_SOURCES}
    ${EDITOR_HEADERS}
)

source_group("Editor\\TextEditor" FILES
    text_editor.cpp text_editor.h
)

source_group("Editor\\Syntax" FILES
    syntax_highlighter.cpp syntax_highlighter.h
)

# Project Management (项目管理模块)
source_group("Project Management" FILES
    ${PROJECT_MANAGEMENT_SOURCES}
    ${PROJECT_MANAGEMENT_HEADERS}
    ${PROJECT_MANAGEMENT_FORMS}
)

source_group("Project Management\\Config" FILES
    project_config.cpp project_config.h
)

source_group("Project Management\\Dialog" FILES
    project_creation_dialog.cpp project_creation_dialog.h project_creation_dialog.ui
    project_selection_dialog.cpp project_selection_dialog.h
)

source_group("Project Management\\Manager" FILES
    project_manager.cpp project_manager.h
)

source_group("Project Management\\Panel" FILES
    project_panel.cpp project_panel.h
)

# Compile System (编译系统模块)
source_group("Compile System" FILES
    # ${COMPILE_SYSTEM_SOURCES}
    # ${COMPILE_SYSTEM_HEADERS}
)

# Resources (资源文件)
source_group("Resources" FILES ${RESOURCE_FILES})
source_group("Resources\\Icons" FILES
    resources/lightide_resources.qrc
)
source_group("Resources\\Fonts" FILES
    resources/fonts.qrc
)

# Run Module (运行模块)
source_group("Run" FILES ${JAVA_SOURCES} ${JAVA_HEADERS})
source_group("Run\\Configuration" FILES
    src/run/run_configuration.cpp src/run/run_configuration.h
    src/run/run_config_manager.cpp src/run/run_config_manager.h
)
source_group("Run\\Launcher" FILES
    src/run/java_launcher.cpp src/run/java_launcher.h
)

# Java Module (Java 支持模块)
source_group("Java" FILES ${JAVA_SOURCES} ${JAVA_HEADERS})
source_group("Java\\Model" FILES
    src/java/project_model.cpp src/java/project_model.h
    src/java/maven_project_model.cpp src/java/maven_project_model.h
    src/java/gradle_project_model.cpp src/java/gradle_project_model.h
    src/java/plain_java_project_model.cpp src/java/plain_java_project_model.h
)
source_group("Java\\Resolver" FILES
    src/java/project_resolver.cpp src/java/project_resolver.h
)

# 设置包含路径
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# macOS特定设置
if(APPLE)
    set(MACOSX_ICON_FILE "${CMAKE_CURRENT_SOURCE_DIR}/resources/app_icon.icns")

    if(EXISTS "${MACOSX_ICON_FILE}")
        # 关键：告诉 CMake 用哪个图标 + 把它拷进 .app/Contents/Resources/
        set_target_properties(${PROJECT_NAME} PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_GUI_IDENTIFIER "org.sprintide.app"
            MACOSX_BUNDLE_BUNDLE_NAME "${APP_NAME}"
            MACOSX_BUNDLE_BUNDLE_VERSION "${APP_VERSION}"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "${APP_VERSION}"
            MACOSX_BUNDLE_ICON_FILE "app_icon.icns"    # ★ 新增
        )
        # 把 icns 加到 bundle 资源列表
        target_sources(${PROJECT_NAME} PRIVATE "${MACOSX_ICON_FILE}")
        set_source_files_properties("${MACOSX_ICON_FILE}" PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources"
        )
    else()
        message(WARNING "resources/app_icon.icns 不存在，.app 将显示默认图标")
        set_target_properties(${PROJECT_NAME} PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_GUI_IDENTIFIER "org.sprintide.app"
            MACOSX_BUNDLE_BUNDLE_NAME "${APP_NAME}"
            MACOSX_BUNDLE_BUNDLE_VERSION "${APP_VERSION}"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "${APP_VERSION}"
        )
    endif()

    # 原有的额外框架
    target_link_libraries(${PROJECT_NAME} PRIVATE
        "-framework AppKit"
        "-framework Foundation"
    )
endif()

# Windows特定设置
if(WIN32)
    # 添加Windows版本信息
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/version.rc")
        target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/version.rc")
    endif()

    # 设置Windows可执行文件图标
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )

    # 性能监控需要 psapi
    target_link_libraries(${PROJECT_NAME} PRIVATE psapi)
endif()

# 安装目标
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# 头文件检查
foreach(HEADER_FILE ${ALL_HEADERS})
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${HEADER_FILE}")
        message(WARNING "头文件不存在: ${HEADER_FILE}")
    endif()
endforeach()

# 源文件数量
list(LENGTH ALL_SOURCES TOTAL_SOURCES_COUNT)
list(LENGTH CORE_SOURCES CORE_COUNT)
list(LENGTH UI_SOURCES UI_COUNT)
list(LENGTH EDITOR_SOURCES EDITOR_COUNT)
list(LENGTH PROJECT_MANAGEMENT_SOURCES PROJECT_COUNT)

# 显示构建信息

message(STATUS "==========================================")
message(STATUS "SprintIDE 构建配置信息:")
message(STATUS "  项目名称: ${PROJECT_NAME}")
message(STATUS "  版本: ${PROJECT_VERSION}")
message(STATUS "  C++标准: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Qt版本: ${Qt6_VERSION}")
message(STATUS "  核心框架模块: ${CORE_COUNT} 个源文件")
message(STATUS "  界面模块: ${UI_COUNT} 个源文件")
message(STATUS "  编辑器模块: ${EDITOR_COUNT} 个源文件")
message(STATUS "  项目管理模块: ${PROJECT_COUNT} 个源文件")
message(STATUS "     - 项目配置系统 (project_config)")
message(STATUS "     - 项目创建向导 (project_creation_dialog)")
message(STATUS "     - 项目文件树面板 (project_panel)")
message(STATUS "  编译系统模块: ${COMPILE_SYSTEM_SOURCES} 个源文件 (待实现)")
message(STATUS "  总源文件数: ${TOTAL_SOURCES_COUNT} 个")
message(STATUS "==========================================")

# 添加构建后步骤：提示用户下载字体
# add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#     COMMENT "注意: 如需使用JetBrains Mono字体，请从 https://github.com/JetBrains/JetBrainsMono/releases 下载并放置在 resources/ 目录下"
# )


# ============================================================
# macOS 打包：make deploy → 把 Qt 依赖库塞进 SprintIDE.app
# ============================================================
if(APPLE)
    # 定位 macdeployqt：优先用 Qt6 安装目录下的 bin，其次走 PATH
    find_program(MACDEPLOYQT_EXECUTABLE
        NAMES macdeployqt
        HINTS
            "${Qt6_DIR}/../../../bin"
            "${Qt6_DIR}/../../bin"
        REQUIRED
    )
    message(STATUS "macdeployqt: ${MACDEPLOYQT_EXECUTABLE}")

    add_custom_target(deploy
        COMMAND "${MACDEPLOYQT_EXECUTABLE}"
                "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>"
                -verbose=2
        DEPENDS ${PROJECT_NAME}
        COMMENT "Deploying Qt frameworks into SprintIDE.app"
        USES_TERMINAL
    )
endif()
