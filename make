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
    if [[ ! "${notests:-}" ]]; then
        run_tests
        docs
    fi
}

sub_compile() {
    rm -fr ./*.o lc

    nasm -f elf64 -o lc_asm.o lc.asm

    echo "#define INITIAL_CODE \"$(
        cat lc.lc |
        egrep -v '^ *#' |
        tr '\n' ' ' |
        sed -e 's/\s\+/ /g' |
        sed 's/ (/(/g' |
        sed 's/) /)/g' |
        sed 's/) )/))/g' |
        sed 's/( (/((/g' |
        sed 's/"/\\"/g'
    )\"" >lc.lc.h

    if [[ ${debug:-} ]]; then
        gcc -Os -g -c lc.c
        ld -o lc -dynamic-linker /lib64/ld-linux-x86-64.so.2 -lc -ldl --entry=main ./*.o
    else
        gcc -Os -c lc.c
        ld -o lc -dynamic-linker /lib64/ld-linux-x86-64.so.2 -lc -ldl --entry=main ./*.o
        strip -R .eh_frame -R .gnu.version -R .hash -R .comment --strip-all lc
    fi

    rm -fr ./*.o
}

run_tests() {
    for test_file in tests/*.{lc,in}; do
        if [[ ! "$regex" ]] || egrep "$regex" <<<"$test_file" &>/dev/null; then
            run_test "$test_file"
        fi
    done

    tests_done

    if [[ ${all:-} ]]; then
        check_summation 50 1000 200
    fi
}

run_test() {
    local file="$1"
    local filename="$(basename "$file")"
    local extension="${filename##*.}"
    local input="${filename%.*}"

    local args=(
        "$input"

        "$(
            if [[ -e "tests/${input}.out" ]]; then
                cat "tests/${input}.out"
            else
                ./lc < "tests/${input}.eout" 2>/dev/null
            fi
        )"

        "$(
            mkdir -p tmp_test_dir
            if [[ $extension == lc ]]; then
                ./lc "$file" 2>/dev/null
            else
                ./lc < "$file" 2>/dev/null
            fi
            rm -fr tmp_test_dir
        )"

        "$(
            mkdir -p tmp_test_dir
            if [[ $extension == lc ]]; then
                ./lc "$file" 2>&1
            else
                ./lc < "$file" 2>&1
            fi
            rm -fr tmp_test_dir
        )"
    )

    assert_output "${args[@]}"
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
        elif [[ "$nr" != "0" && "$(( RANDOM % 3 ))" == "0" ]]; then
            echo -n " (+ $nr 0 0 0 0 0)"
        elif [[ "$nr" != "0" && "$(( RANDOM % 2 ))" == "0" ]]; then
            echo -n " (+ (- $nr 1) 1)"
        elif [[ "$nr" != "0" && "$(( RANDOM % 2 ))" == "0" ]]; then
            echo -n " (/ (* $nr 2) 2)"
        elif [[ "$(( RANDOM % 2 ))" == "0" ]]; then
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

docs() {
    mkdir -p dist
    ./lc <<<"(doc-code 'lc.lc' 'dist/index.html')" || echo 'Failed docs.'
}

main "$@"
