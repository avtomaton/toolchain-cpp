#create a pretty commit id using git
#uses 'git describe --tags', so tags are required in the repo
#create a tag with 'git tag <name>' and 'git push --tags'

find_package(Git)
if(GIT_FOUND)
    execute_process(COMMAND ${GIT_EXECUTABLE} describe --tags RESULT_VARIABLE res_var OUTPUT_VARIABLE GIT_COM_ID )
    if( NOT ${res_var} EQUAL 0 )
        set( GIT_COMMIT_ID "git commit id unknown")
        message( WARNING "Git failed (not a repo, or no tags). Build will not contain git revision info." )
    endif()
    string( REPLACE "\n" "" GIT_COMMIT_ID ${GIT_COM_ID} )
else()
    set( GIT_COMMIT_ID "unknown (git not found!)")
    message( WARNING "Git not found. Build will not contain git revision info." )
endif()

set( vstring "//version_string.hpp - written by cmake. changes will be lost!\n"
             "const char * VERSION_STRING = \"${GIT_COMMIT_ID}\"\;\n")

file(WRITE version_string.hpp.txt ${vstring} )
# copy the file to the final header only if the version changes
# reduces needless rebuilds
execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        version_string.hpp.txt ${CMAKE_CURRENT_BINARY_DIR}/version_string.hpp)
