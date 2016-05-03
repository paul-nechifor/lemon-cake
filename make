#!/bin/bash -e

root="$(cd "$(dirname "$BASH_SOURCE")"; pwd)"

main() {
    cd "$root"
    compile_all
    run_tests
    echo "Binary size: $(wc -c < lc)"
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
    echo -n "Testing $(tr '\n' ' ' <<<"$input") ... "
    local output="$(./lc <<<"$input" 2>/dev/null)"
    if [[ "$expected" == "$output" ]]; then
        echo "$(tput setaf 2)ok$(tput sgr0)"
    else
        echo "$(tput setaf 1)failed$(tput sgr0)"
        echo "    expected: $(tput setab 2)$expected$(tput sgr0)"
        echo "    actual: $(tput setab 1)$output$(tput sgr0)"
    fi
}

main "$@"
