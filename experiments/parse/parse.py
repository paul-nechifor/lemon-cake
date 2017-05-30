#!/usr/bin/env python2

import json
from unittest import TestCase, main


def cs(s):
    """CoffeeScript like strings."""
    lines = s.split('\n')
    min_indent = len(lines[-1]) + 4
    return '\n'.join(x[min(min_indent, len(x)):] for x in lines[1:-1])


class TestCs(TestCase):

    def test_cs01(self):
        self.assertEqual('abc', cs('''
            abc
        '''))

    def test_cs02(self):
        self.assertEqual('abc\n\nc', cs('''
            abc

            c
        '''))


main()

exit(0)


operators = [
    '=',
    '~',
    '*',
    '/',
    '+',
    '-',
]

tests = [['''\
###############################################################################
1 + 2 + 3
''', '''\
(+ 1 2 3)
'''], ['''\
###############################################################################
1 + 2 * 3
''', '''\
(+ 1 (* 2 3))
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
(= fn (~ (x) (+ x 1)))
'''], ['''\
###############################################################################
fn = x ~
  x + 1
''', '''\
(= fn (~ (x) (+ x 1)))
'''], ['''\
###############################################################################
fn = x ~

  add

    x - 2


    1
''', '''\
(= fn (~ (x) (add x 1)))
'''], ['''\
###############################################################################
a = f1 f2 f3 1 + 2 * 3 * 4 + 3 / 2 * 4
''', '''\
(= a (f1 (f2 (f3 (+ 1 (* 2 3 4) (* (/ 3 2)))))))
''']]


def parse(text):
    print '=' * 100
    expr = process_indents(text)
    show(expr)
    print '-' * 100
    tokens = tokenize(expr)
    show(tokens)
    print '-' * 100
    grouped = group_tokens(tokens)
    show(grouped)
    print '-' * 100
    rep = repr_grouped(grouped)
    print rep
    print '-' * 100
    print '\n' * 4
    return rep


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
        expr = ('unparsed', trimmed_line, [])
        indent_stack[indent] = expr
        if indent == 0:
            expressions.append(expr)
        else:
            indent_stack[indent - 1][2].append(expr)

        prev_indent = indent
    if len(expressions) == 1:
        return expressions[0]
    return ('unparsed', 'last', expressions)


def tokenize(expr):
    return tokenize_str(expr[1]) + tokenize_blocks(expr[2])


def tokenize_str(s):
    return [('token', e) for e in s.split(' ')]


def tokenize_blocks(blocks):
    return [
        ('group', tokenize(b))
        for b in blocks
    ]


def group_tokens(tokens):
    if len(tokens) == 1:
        return tokens[0]
    for op in operators:
        indices = get_indices(tokens, op)
        if indices:
            return ('list', [('token', op)] + get_list(tokens, indices))

    return ('list', [x['token'] for x in tokens])


def get_list(tokens, indices):
    indices = [-1] + indices + [len(tokens)]
    return [
        group_tokens(tokens[indices[i - 1] + 1:indices[i]])
        for i in range(1, len(indices))
    ]


def get_indices(tokens, op):
    return [
        i
        for i, x in enumerate(tokens)
        if x[0] == 'token' and x[1] == op
    ]


def repr_grouped(g):
    if g[0] == 'list':
        return '(%s)' % (' '.join(repr_grouped(x) for x in g[1]))
    if g[0] == 'token':
        return g[1]
    raise Exception('Handle %s' % repr(g))


def test():
    for i, (in_text, out_text) in enumerate(tests[0:2]):
        if parse(in_text) != out_text:
            print '%i failed' % i


# test()


"""
exp ::= term  | exp + term | exp - term
term ::= factor | factor * term | factor / term
factor ::= number | ( exp )
"""

class TokenList(object):

    def __init__(self, toks):
        self.toks = toks
        self.i = 0
        self.tok = self.toks[self.i]

    def next(self):
        self.i += 1
        if self.i >= len(self.toks):
            self.tok = None
        else:
            self.tok = self.toks[self.i]


def exp(t):
    ret = term(t)
    while t.tok in ('+', '-'):
        token = t.tok
        t.next()
        ret = [token, ret, term(t)]
    return ret


def factor(t):
    if t.tok == '(':
        t.next()
        ret = exp(t)
        t.next()
    else:
        ret = t.tok
        t.next()
    return ret


def term(t):
    ret = factor(t)
    while t.tok in ('*', '/'):
        token = t.tok
        t.next()
        ret = [token, ret, term(t)]
    return ret


def tokenise_str(s):
    lst = list(s.replace(' ', ''))
    tokens = []
    for i in range(len(lst)):
        if lst[i].isdigit() and i > 0 and  (tokens[-1].isdigit() or tokens[-1][-1] is '.'):
            tokens[-1] += lst[i]
        elif lst[i] is '.' and i > 0 and tokens[-1].isdigit():
            tokens[-1] += lst[i]
        else:
            tokens.append(lst[i])
    return tokens


def create_ast(s):
    print s
    print exp(TokenList(tokenise_str(s)))


if __name__ == '__main__':
    create_ast('2 + 3 * 4')
    create_ast('(2 + 3) * 4')
