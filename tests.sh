# More tests:
# test_out "(do (multi-assign (a b c) (1 2 3)) (list c a b))" "(3 1 2)"
# test_out "((func 4))" "4"
# test_out "((func (+ 1 (head args))) 2 3 4)" "3"

# Long tests.
random_plus_expression() {
    local left="$1" nr=
    echo -n '(+'
    while [[ "$nr" != "0" ]]; do
        nr=$(( RANDOM % left ))
        left=$(( left - nr ))
        if [[ "$nr" != "0" && "$(( RANDOM % 6 ))" == "0" ]]; then
            echo -n ' '
            random_plus_expression "$nr"
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

if [[ $all ]]; then
    check_summation 50 1000 200
fi
