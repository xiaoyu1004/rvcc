set(cmd "/bin/bash ${PROJECT_SOURCE_DIR}/script/exec.sh")

add_test(NAME RvccTest COMMAND ${cmd})