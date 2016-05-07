test_out '4' '4'

test_out '1234
5' '1234
5'

test_out '"asdf"' '"asdf"'

test_out '321
"str"
123' '321
"str"
123'

# Test whitespace.
test_out '  1234 ' '1234'
test_out '     "a" ' '"a"'

# Test comments.
test_out '"b"#c' '"b"'
test_out ' "bc" #d' '"bc"'
test_out ' "bcd" #asdf
8' '"bcd"
8'

# Test null.
test_out '' 'null'
test_out '# Just a comment.' 'null'
test_out '    # Indented comment.' 'null'

# Test lists.
test_out '()' '()'
test_out '  () # asdf' '()'
test_out '(1)' '(1)'
test_out '(1 )' '(1)'
test_out '( 22 )' '(22)'
test_out '  ( 22 ) ' '(22)'
test_out '(1 2 3)' '(1 2 3)'
test_out '(1 "asdf" 3)' '(1 "asdf" 3)'
test_out '(1 (22 33) 3)' '(1 (22 33) 3)'
test_out '((1 "df" ("asf") () ("bb" ) ) 123)' '((1 "df" ("asf") () ("bb")) 123)'

tests_done
