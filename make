#!/bin/bash -e

root="$(cd "$(dirname "$BASH_SOURCE")"; pwd)"
n_tests=0 n_ok=0 n_failed=0

main() {
    cd "$root"
    if [[ "$1" ]]; then
        sub_"$@"
        return
    fi
    sub_compile
    run_tests
    echo "Binary size: $(wc -c < lc)"
}

sub_compile() {
    rm -fr *.o lc
    nasm -f elf64 lc.asm
    gcc -Os -c lc_c.c
    ld -o lc -dynamic-linker /lib64/ld-linux-x86-64.so.2 -lc -ldl --entry=main *.o
    if [[ ! "$debug" ]]; then
        strip -R .eh_frame -R .gnu.version -R .hash -R .comment --strip-all lc
    fi
    rm -fr *.o
}

run_tests() {
    . "$root/tests.sh"
}

test_out() {
    local input="$1" expected="$2"
    echo -n "${n_tests}. Testing $(af 4)$(tr '\n' ' ' <<<"$input")$(sgr) ... "
    ((n_tests++)) || true
    local output="$(./lc <<<"$input" 2>/dev/null)"
    if [[ "$expected" == "$output" ]]; then
        echo "$(af 2)ok$(sgr)"
        ((n_ok++)) || true
    else
        echo "$(af 1)failed$(sgr)"
        echo "    $(af 2)expected$(sgr): $(ab 2)$expected$(sgr)"
        echo "    $(af 1)actual$(sgr): $(ab 1)$output$(sgr)"
        ((n_failed++)) || true
    fi
}

tests_done() {
    echo "results: ok($n_ok), failed($n_failed)"
}

sgr() { tput sgr0; }
af() { tput setaf "$@"; }
ab() { tput setab "$@"; }

main "$@"
