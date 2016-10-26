#!/bin/bash -e

root="$(cd "$(dirname "$BASH_SOURCE")"; pwd)"
n_tests=0
n_ok=0
n_failed=0

main() {
    cd "$root"
    if [[ "$1" ]]; then
        sub_"$@"
        return
    fi
    sub_compile
    [[ "${notests:-}" ]] || run_tests
}

sub_compile() {
    rm -fr ./*.o lc

    nasm -f elf64 lc.asm

    echo "#define INITIAL_CODE \"$(
        tr '\n' ' ' <lc.lc | sed -e 's/\s\+/ /g'
    )\"" >lc.lc.h

    if [[ ${debug:-} ]]; then
        gcc -Os -g -c lc_c.c
        ld -o lc -dynamic-linker /lib64/ld-linux-x86-64.so.2 -lc -ldl --entry=main ./*.o
    else
        gcc -Os -c lc_c.c
        ld -o lc -dynamic-linker /lib64/ld-linux-x86-64.so.2 -lc -ldl --entry=main ./*.o
        strip -R .eh_frame -R .gnu.version -R .hash -R .comment --strip-all lc
    fi

    rm -fr ./*.o
}

run_tests() {
    for test_file in tests/*.lc; do
        local base_name="${test_file%.lc}"
        assert_output \
            "$(basename "$test_file" .lc)" \
            "$(cat "$base_name".out)" \
            "$(./lc "$test_file" 2>/dev/null)" \
            "$(./lc "$test_file" 2>&1)"
    done

    for test_file in tests/*.in; do
        local base_name="${test_file%.in}"
        assert_output \
            "$(basename "$test_file" .in)" \
            "$(cat "$base_name".out)" \
            "$(./lc < "$test_file" 2>/dev/null)" \
            "$(./lc < "$test_file" 2>&1)"
    done

    tests_done

    if [[ ${all:-} ]]; then
        check_summation 50 1000 200
    fi
}

assert_output() {
    local name="$1"
    local expected="$2"
    local actual="$3"
    local full_output="$4"

    local input="$1" expected="$2"
    ((n_tests++)) || true
    echo -n "${n_tests}. Testing $(af 4)$name$(sgr) ... "
    local output=
    if [[ "$expected" == "$actual" ]]; then
        echo "$(af 2)ok$(sgr)"
        ((n_ok++)) || true
    else
        echo "$(af 1)failed$(sgr)"
        echo "    $(af 2)expected$(sgr): $(ab 2)$expected$(sgr)"
        echo "    $(af 1)actual$(sgr): $(ab 1)$full_output$(sgr)"
        ((n_failed++)) || true
    fi
}

tests_done() {
    echo "results: ok($n_ok), failed($n_failed)"
    echo "Binary size: $(wc -c < lc)"
}

sgr() {
    tput sgr0;
}

af() {
    tput setaf "$@";
}

ab() {
    tput setab "$@";
}

random_plus_expression() {
    local left="$1" nr=
    echo -n '(+'
    while [[ "$nr" != "0" ]]; do
        nr=$(( RANDOM % left ))
        left=$(( left - nr ))
        if [[ "$nr" != "0" && "$(( RANDOM % 6 ))" == "0" ]]; then
            echo -n ' '
            random_plus_expression "$nr"
        elif [[ "$(( RANDOM % 4 ))" == "0" ]]; then
            echo -n " ((~ $nr))"
        else
            echo -n " $nr"
        fi
    done
    echo -n " $left)"
}

check_summation() {
    local n_times="$1" n_buffer_lines="$2" number="$3"
    local string="$(
        for (( i=0; i<n_buffer_lines; i++ )); do
            random_plus_expression "$number"
            echo
        done
    )"
    echo -e "\n\nMemory usage:\n"
    (
        for (( i=0; i<n_times; i++ )); do
            echo "$string"
        done
    ) | ./lc 2>/dev/null | (
        local pid="$(ps aux | grep './lc' | grep -v grep | awk '{print $2}')"
        local i=0
        while read line; do
            let i++ || true
            if [[ "$i" == "$n_buffer_lines" ]]; then
                let i=0 || true
                ps aux | grep "$pid" | grep -v grep | awk '{print $5}'
            fi

            if [[ "$line" != "$number" ]]; then
                echo "Failed!"
                exit 1
            fi
        done
    )
}

main "$@"
