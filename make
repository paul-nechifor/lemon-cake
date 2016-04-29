#!/bin/bash -e

root="$(cd "$(dirname "$BASH_SOURCE")"; pwd)"

main() {
    cd "$root"
    compile_all
    run_tests
    wc -c < lc
}

compile_all() {
    rm -fr *.o lc
    nasm -f elf64 lc.asm
    gcc -c lc_c.c
    ld -o lc -dynamic-linker /lib64/ld-linux-x86-64.so.2 -lc --entry=main *.o
    rm -fr *.o
    strip -R .eh_frame -R .gnu.version -R .hash -R .comment --strip-all lc
}

run_tests() {
    . "$root/tests.sh"
}

test_out() {
    local input="$1" expected="$2"
    echo -n "Testing $input ... "
    local output="$(./lc <<<"$input")"
    if diff <(echo "$expected") <(echo "$output") &>/dev/null; then
        echo 'ok'
    else
        echo -e "failed\n    expected: '$expected'\n    actual: '$output'"
    fi
}

main "$@"
