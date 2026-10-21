file(GLOB_RECURSE cerfqt_bundled RELATIVE "${CERFQT_BUNDLED_DIR}" "${CERFQT_BUNDLED_DIR}/*")
foreach(cerfqt_file IN LISTS cerfqt_bundled)
    if(cerfqt_file MATCHES "(^|/)\\.git(/|$)|(^|/)\\.gitkeep$|\\.(i64|id0|id1|id2|nam|til|cmd)$")
        continue()
    endif()
    get_filename_component(cerfqt_dir "${CERFQT_OUTPUT_DIR}/${cerfqt_file}" DIRECTORY)
    file(MAKE_DIRECTORY "${cerfqt_dir}")
    file(COPY_FILE "${CERFQT_BUNDLED_DIR}/${cerfqt_file}" "${CERFQT_OUTPUT_DIR}/${cerfqt_file}"
         ONLY_IF_DIFFERENT)
    if(cerfqt_file MATCHES "\\.sh$")
        file(CHMOD "${CERFQT_OUTPUT_DIR}/${cerfqt_file}"
             FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                              GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    endif()
endforeach()
file(COPY_FILE "${CERFQT_DB_JSON}" "${CERFQT_OUTPUT_DIR}/db.json" ONLY_IF_DIFFERENT)
