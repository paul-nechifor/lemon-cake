tokenize = (chars) ->
  chars
  .replace /\(/g, ' ( '
  .replace /\)/g, ' ) '
  .trim()
  .split /\s+/

parse = (program) ->
  readFromTokens tokenize program

readFromTokens = (tokens) ->
  throw 'unexpected EOF while reading' if tokens.length is 0
  token = tokens.shift()
  if token is '('
    l = []
    while tokens[0] isnt ')'
      l.push readFromTokens tokens
    tokens.shift()
    return l
  else if token is '('
    throw 'unexpected )'
  else
    return atom token
  return

class Symbol
  constructor: (@value) ->

class Lambda
  constructor: (@params, @body, @env) ->

  apply: (obj, args) ->
    # env = new Environment
    env = standardEnvironment()
    for name, i in @params
      env.values[name.value] = if args[i] is undefined then null else args[i]
    evaluateInEnv @body, env

class Environment
  constructor: (@values={}) ->

atom = (token) ->
  n = parseFloat token
  if isNaN(n) then new Symbol token else n

standardEnvironment = ->
  new Environment
    '+': (a, b) -> a + b
    '-': (a, b) -> a - b
    '*': (a, b) -> a * b
    '/': (a, b) -> a / b

evaluateInEnv = (x, env) ->
  # Var reference.
  return env.values[x.value] if x instanceof Symbol
  # Constant.
  return x unless x instanceof Array
  return null if x.length is 0
  switch x[0].value
    when 'quote' then return x[1]
    when '~' then return new Lambda x[1], x[2], env
    when '=' then return env.values[x[1].value] = evaluateInEnv x[2], env
    when 'do'
      return false if x.length is 1
      for x2 in x.slice 1
        last = evaluateInEnv x2, env
      return last
    when 'if' then return evaluateInEnv (
      if evaluateInEnv x[1] then x[2] else x[3]
    ), env
  proc = evaluateInEnv x[0], env
  args = (evaluateInEnv(x2, env) for x2 in x.slice(1))
  proc.apply null, args

defaultEnvironment = standardEnvironment()

evaluate = (x) ->
  evaluateInEnv x, defaultEnvironment

module.exports =
  Environment: Environment
  Symbol: Symbol
  atom: atom
  evaluateInEnv: evaluateInEnv
  parse: parse
  standardEnvironment: standardEnvironment
  tokenize: tokenize
