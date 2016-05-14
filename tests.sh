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
test_out '  12
   12345  \

       55    ' '12
12345
()
55'

# Test comments.
test_out '"b"#c' '"b"'
test_out ' "bc" #d' '"bc"'
test_out ' "bcd" #asdf
8' '"bcd"
8'

# Test null.
test_out '' '()'
test_out '# Just a comment.' '()'
test_out '    # Indented comment.' '()'

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

# Test symbols.
test_out 'asdf' 'asdf'
test_out '+' '+'
test_out '<=>' '<=>'
test_out '(asdf)' '(asdf)'
test_out '(asdf 1 ab)' '(asdf 1 ab)'
test_out '(a s d f "x" (1 ab))' '(a s d f "x" (1 ab))'
test_out '(xx "yy")' '(xx "yy")'
test_out '(a b)
(a b)
(a b)' '(a b)
(a b)
(a b)'
test_out '(aa bb cc)
  (a c)
( a b c ) # Nothing here
(cc dd)' '(aa bb cc)
(a c)
(a b c)
(cc dd)'

# Test add.
test_out '(+ 1 2)' '3'
test_out '(+ 8 4)' '12'
test_out '(+ 1 1)
(+ 2 3)
 ( + 44 11 ) #
' '2
5
55
()'
test_out '(+ 1 2 3 4 5)
(+ 3 3 3)' '15
9'

# Test nested calls
test_out '(+ 1 2 (+ 3))' '6'
test_out '(+ 1 2 (+ 3 4))' '10'
test_out '(+ 1 1 (+ 1 (+ 1 (+ 1 1) 1 (+ 1 1))))' '9'

tests_done
