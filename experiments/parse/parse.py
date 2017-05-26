#!/usr/bin/env python2

import json

tests = [['''\
###############################################################################
1 + 2 + 3
''', '''\
(+ 1 2 3)
'''], ['''\
###############################################################################
1 + 2 * 3
''', '''\
(+ 1 (* 2 3)
'''], ['''\
###############################################################################
1 + 2
3
5 * 3
''', '''\
(last (+ 1 2) 3 (* 5 3))
'''], ['''\
###############################################################################
a = ~ x
''', '''\
(= (~ x)
'''], ['''\
###############################################################################
fn = x ~ x + 1
''', '''\
(= fn (~ (x) (+ x 1)
'''], ['''\
###############################################################################
fn = x ~
  x + 1
''', '''\
(= fn (~ (x) (+ x 1)
'''], ['''\
###############################################################################
fn = x ~

  add

    x - 2


    1
''', '''\
(= fn (~ (x) (add x 1)
''']]


def parse(text):
    print '=' * 100
    expr = process_indents(text)
    show(expr)
    print '-' * 100
    tokens = tokenize(expr)
    show(tokens)
    print '\n' * 4


def show(x):
    print json.dumps(x, sort_keys=True, separators=(',', ':'), indent=2)


def process_indents(text):
    lines = filter(None, [x.rstrip() for x in text.split('\n')])
    lines = [x for x in lines if x[0] != '#']
    expressions = []
    prev_indent = 0
    indent_stack = {}
    for line in lines:
        trimmed_line = line.lstrip()
        indent = (len(line) - len(trimmed_line)) / 2
        expr = {
            'type': 'unparsed-expression',
            'expr': trimmed_line,
            'blocks': [],
        }
        indent_stack[indent] = expr
        if indent == 0:
            expressions.append(expr)
        else:
            indent_stack[indent - 1]['blocks'].append(expr)

        prev_indent = indent
    if len(expressions) == 1:
        return expressions[0]
    return {
        'type': 'unparsed-expression',
        'expr': 'last',
        'blocks': expressions,
    }


def tokenize(expr):
    return tokenize_str(expr['expr']) + tokenize_blocks(expr['blocks'])


def tokenize_str(s):
    return [
        {'type': 'token', 'str': e}
        for e in s.split(' ')
    ]


def tokenize_blocks(blocks):
    return [
        {
            'type': 'group',
            'tokens': tokenize(b)
        }
        for b in blocks
    ]


def test():
    for i, (in_text, out_text) in enumerate(tests):
        if parse(in_text) != out_text:
            print '%i failed' % i

test()
