# Development signing only, after all bundle relocation. Not notarization.
if(NOT DEFINED APP OR NOT EXISTS "${APP}/Contents/Info.plist")
    message(FATAL_ERROR "A complete Cemu application bundle is required")
endif()
file(GLOB libraries "${APP}/Contents/Frameworks/*.dylib")
foreach(image IN LISTS libraries)
    execute_process(COMMAND /usr/bin/codesign --force --sign - --timestamp=none "${image}"
        COMMAND_ERROR_IS_FATAL ANY)
endforeach()
execute_process(COMMAND /usr/bin/codesign --force --sign - --timestamp=none "${APP}"
    COMMAND_ERROR_IS_FATAL ANY)
execute_process(COMMAND /usr/bin/codesign --verify --deep --strict "${APP}"
    COMMAND_ERROR_IS_FATAL ANY)
