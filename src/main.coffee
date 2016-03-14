readline = require 'readline'
{evaluateInEnv, parse, standardEnvironment} = require './index'

module.exports = ->
  env = standardEnvironment()
  readline.createInterface
    input: process.stdin
    output: process.stdout
    terminal: true
  .on 'line', (line) ->
    console.log evaluateInEnv parse(line), env
