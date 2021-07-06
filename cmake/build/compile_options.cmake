# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

function(az_add_compile_options _TARGET)
  if(MSVC)
    if(WARNINGS_AS_ERRORS)
      set(WARNINGS_AS_ERRORS_FLAG "/WX")
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /WX")
    endif()

    target_compile_options(${_TARGET} PRIVATE /W4 ${WARNINGS_AS_ERRORS_FLAG} /wd5031 /wd4668 /wd4820 /wd4255 /wd4710 /analyze)

    target_compile_options(${_TARGET} PRIVATE
      $<$<CONFIG:>:/MT> #---------|
      $<$<CONFIG:Debug>:/MTd> #---|-- Statically link the runtime libraries
      $<$<CONFIG:Release>:/MT> #--|
    )
  elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
    if(WARNINGS_AS_ERRORS)
      set(WARNINGS_AS_ERRORS_FLAG "-Werror")
    endif()

    if(check_docs)
      target_compile_options(${_TARGET} PRIVATE -Wdocumentation -Wdocumentation-unknown-command)
    endif()
    target_compile_options(${_TARGET} PRIVATE -Xclang -Wall -Wextra -pedantic  ${WARNINGS_AS_ERRORS_FLAG} -fcomment-block-commands=retval -Wunused -Wuninitialized -Wmissing-declarations -Wconversion -Wpointer-arith -Wshadow -Wfloat-equal)
  else()
    if(WARNINGS_AS_ERRORS)
      set(WARNINGS_AS_ERRORS_FLAG "-Werror")
    endif()

    target_compile_options(${_TARGET} PRIVATE -Wall -Wextra -pedantic  ${WARNINGS_AS_ERRORS_FLAG} -Wunused -Wuninitialized -Wmissing-declarations -Wconversion -Wpointer-arith -Wshadow -Wlogical-op -Wfloat-equal)
  endif()
endfunction()
