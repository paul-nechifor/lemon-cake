test_out "4" "4"

test_out "1234
5" "1234
5"

test_out "'asdf'" "'asdf'"

test_out "321
'str'
123" "321
'str'
123"

# Test whitespace.
test_out "  1234 " "1234"
test_out "     'a' " "'a'"
test_out "  12
   12345  \

       55    " "12
12345
55"

# Test comments.
test_out "'b'#c" "'b'"
test_out " 'bc' #d" "'bc'"
test_out " 'bcd' #asdf
8" "'bcd'
8"

# Test null.
test_out "" "()"
test_out "# Just a comment." "()"
test_out "    # Indented comment." "()"

# Test lists.
test_out "()" "()"
test_out "  () # asdf" "()"
test_out "(list )" "()"
test_out "(list 1)" "(1)"
test_out "(list 1 )" "(1)"
test_out "( list 22 )" "(22)"
test_out "  ( list 22 ) " "(22)"
test_out "(list 1 2 3)" "(1 2 3)"
test_out "(list 1 'asdf' 3)" "(1 'asdf' 3)"
test_out "(list 1 (list 22 33) 3)" "(1 (22 33) 3)"
test_out "(list (list 1 'df' (list 'asf') () (list 'bb' ) ) 123)" "((1 'df' ('asf') () ('bb')) 123)"

# Test symbols.
test_out "(quote asdf)" "asdf"
test_out "(quote <=>)" "<=>"
test_out "(list (quote asdf))" "(asdf)"
test_out "(list (quote asdf) 1 (quote ab))" "(asdf 1 ab)"
test_out "(list (quote a) (quote s) (quote d) (quote f) 'x' (list 1 (quote ab)))" "(a s d f 'x' (1 ab))"
test_out "(list (quote xx) 'yy')" "(xx 'yy')"
test_out "(list (quote a) (quote b))
(list (quote a) (quote b))
(list (quote a) (quote b))" "(a b)
(a b)
(a b)"
test_out "(list (quote aa) (quote bb) (quote cc))
  (list (quote a) (quote c))
( list (quote a) (quote b) (quote c) ) # Nothing here
(list (quote cc) (quote dd))" "(aa bb cc)
(a c)
(a b c)
(cc dd)"

# Test add.
test_out "(+ 1 2)" "3"
test_out "(+ 8 4)" "12"
test_out "(+ 1 1)
(+ 2 3)
 ( + 44 11 ) #
" "2
5
55
()"
test_out "(+ 1 2 3 4 5)
(+ 3 3 3)" "15
9"
test_out "(+ 2)
(+ 1 1 1 (+ 1 1))" "2
5"

# Test nested calls
test_out "(+ 1 2 (+ 3))" "6"
test_out "(+ 1 2 (+ 3 4))" "10"
test_out "(+ 1 1 (+ 1 (+ 1 (+ 1 1) 1 (+ 1 1))))" "9"

test_out "(hashcode 0)" "0"
test_out "(hashcode 1234)" "1234"
test_out "(hashcode 2395873184555)" "2395873184555"
test_out "(hashcode 'a')" "177604"
test_out "(hashcode 'base')" "6382700272"
test_out "(hashcode (quote base))" "6382700272"
test_out "(hashcode 'besa')" "6382695920"
test_out "(hashcode 'Paul Nechifor')" "153306557046378463"

test_out "(is 4 4)" "1"
test_out "(is 4 3)" "0"
test_out "(is '4' 4)" "0"
test_out "(is 05 00005)" "1"
test_out "(is 'asdf' 'asdf')" "1"
test_out "(is 'asde' 'asdf')" "0"
test_out "(is 'asdf' 'asdfe')" "0"

test_out "(len 'a')" "1"
test_out "(len '1234')" "4"
test_out "(len '')" "0"
test_out "(len ())" "0"
test_out "(len (list 1))" "1"
test_out "(len (list 1 2))" "2"
test_out "(len (list 1 2 3))" "3"
test_out "(len (dict))" "0"

test_out "(list 1 2 (quote aaa))" "(1 2 aaa)"
test_out "(list 1 (+ 1 2 3) 'asdf')" "(1 6 'asdf')"

test_out "(dict)" "(dict)"
test_out "(dict (quote a) 1 (quote bb) 8 (quote ccc) 123)" "(dict bb 8 ccc 123 a 1)"

test_out "(set (dict) 'name' 'John')" "(dict 'name' 'John')"
test_out "(set (set (dict) 'name' 'John') 1 2)" "(dict 1 2 'name' 'John')"
test_out "(get (set (dict) 'name' 'John') 'name')" "'John'"
test_out "(get (set (set (dict) (quote a) 1) 'a' 2) 'a')" "2"

test_out "(append (list) 1)" "(1)"
test_out "(append (list 3) 1)" "(3 1)"
test_out "(append (list 1 2) 3)" "(1 2 3)"
test_out "(append () ())" "(())"

test_out "(quote x)" "x"
test_out "(quote (a b))" "(a b)"

test_out "(head (list 0 1 2))" "0"
test_out "(head (list 0 1 2))" "0"
test_out "(tail (list 0 1 2))" "(1 2)"
test_out "(head (tail (list 0 1 2)))" "1"
test_out "(tail (tail (list 0 1 2)))" "(2)"
test_out "(head (tail (tail (list 0 1 2))))" "2"
test_out "(tail (tail (tail (list 0 1 2))))" "()"

test_out "(repr 'one')" "'one'()"

cat /dev/null<<END
test_out "(do (multi-assign (a b c) (1 2 3)) (list c a b))" "(3 1 2)"

test_out "((func 4))" "4"

test_out "((func (+ 1 (head args))) 2 3 4)" "3"
END

tests_done

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
