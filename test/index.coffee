lc = require('../src')
require('chai').should()

s = (x) -> new lc.Symbol x

describe 'lc', ->

  describe 'tokenize', ->

    it 'should tokenize', ->
      lc.tokenize '(begin (define r 10) (* pi (* r r)))'
      .should.deep.equal [
        '(', 'begin', '(', 'define', 'r', '10', ')'
        '(', '*', 'pi', '(', '*', 'r', 'r', ')', ')', ')'
      ]

    it 'should ignore whitespace', ->
      lc.tokenize ' (aa     \tx\t y )'
      .should.deep.equal [ '(', 'aa', 'x', 'y', ')']

  describe 'atom', ->

    it 'should parse numbers', ->
      lc.atom('1.4').should.be.a('number').and.equal 1.4

    it 'should parse symbols', ->
      lc.atom 'asdf'
      .should.be.an.instanceof lc.Symbol
      .and.have.property 'value', 'asdf'

  describe 'parse', ->

     it 'should parse', ->
       lc.parse '(begin (define r 10) (* pi (* r r)))'
       .should.deep.equal [
         s('begin'), [s('define'), s('r'), 10],
         [s('*'), s('pi'), [s('*'), s('r'), s('r')]]
       ]

  describe 'evaluateInEnv', ->

    minEnv = new lc.Environment

    evalInNewEnv = (prog) ->
      env = lc.standardEnvironment()
      lc.evaluateInEnv lc.parse(prog), env

    it 'should evaluate numbers', ->
      lc.evaluateInEnv 1.3, minEnv
      .should.equal 1.3

    it 'should evaluate variables', ->
      lc.evaluateInEnv s('some-num'), new lc.Environment {'some-num': 1.5}
      .should.equal 1.5

    it 'should be able to set values', ->
      lc.evaluateInEnv [s('='), s('abc'), 123], env = new lc.Environment
      env.values.abc.should.equal 123

    it 'should be able to take the first if branch', ->
      lc.evaluateInEnv [s('if'), true, 1, 2], minEnv
      .should.equal 1

    it 'should be able to take the second if branch', ->
      lc.evaluateInEnv [s('if'), false, 1, 2], minEnv
      .should.equal 2

    it 'should have some functions present', ->
      lc.evaluateInEnv s('+'), lc.standardEnvironment()
      .should.be.an.instanceof Function

    it 'should be able to evaluate some functions', ->
      lc.evaluateInEnv [s('+'), 1, 2], lc.standardEnvironment()
      .should.equal 3

    it 'should be able to use variables', ->
      evalInNewEnv '(do (= var 5) (* var 10))'
      .should.equal 50

    it 'should do able to use lambdas', ->
      evalInNewEnv '(do (= ca (~ (r) (* 3.14 (* r r)))) (ca 10)))'
      .should.equal 314
