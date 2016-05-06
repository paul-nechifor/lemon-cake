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

test_out '' 'null'
test_out '# Just a comment.' 'null'
test_out '    # Indented comment.' 'null'

tests_done
