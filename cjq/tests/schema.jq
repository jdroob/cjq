module {
  "name": "schema",
  "description": "JSON Schema Inference",
  "version": "0.0.3.1",
  "homepage": "https://gist.github.com/pkoppstein/a5abb4ebef3b0f72a6ed",
  "license": "MIT",
  "author": "pkoppstein at gmail dot com"
};

# NEWS:
# 2022.10.16
# - unionType need not be top-level
# - documentation, notably mention gojq
# 2021.01.18
# - the extended type of [] is [], and [] U [T] => [T]
# 2021.01.16
# - bug fix @2021
# 2019.07.26
# - every type is regarded as including null if and only if $ARGS.named.nullable (default: false)

# Requires: jq 1.5 or higher, or gojq

# Usage examples:
# jq --argjson nullable true 'include "schema"; schema' input.json
# jq .[] huge.json | jq -n 'include "schema" {search: "."}; schema(inputs)'
# gojq .[] huge.json | gojq -L . -nc 'include "schema"; schema(inputs)'

# Example of use with jm (http://github.com/pkoppstein/jm) 
# jm HUGE.json | jq -n 'include "schema" {search: "."}; schema(inputs)'

# To check conformance with the structural schemas produced by this module,
# see the JESS repository at https://github.com/pkoppstein/JESS

# This module defines four filters:
#   schema/0 returns the typeUnion of the extended-type values of the entities
#     in the input array, if the input is an array;
#     otherwise it simply returns the "typeof" value of its input.
#   schema(stream) returns the schema of the items in the stream;
#   typeof/0 returns the extended-type of its input;
#   typeUnion(a;b) returns b if a == null, otherwise it returns
#     the union of the two specified extended-type values;

# All extended-type values are themselves JSON entities, e.g.
# the JSON value null has extended type `"null"`.

# Each extended type can be thought of as a set of JSON entities,
# e.g. "number" for the set consisting of null and the JSON numbers,
# and ["number"] for the set of arrays whose elements are in "number".

# Note that the JSON value [] can be thought of as belonging to all
# extended types of the form [T] as well as [] and "JSON".

# If $nullable, then `null` is regarded as an element of all types;
# otherwise, null is only regarded as being a member of "null",
# "scalar", "JSON", and any type of the form ["+", "null", X]; and all
# other types, T, are extended by ["+", "null", T].

# The extended types are:
# "null", "boolean", "string", "number";
# "scalar" for any combination of scalars (null, true, false, strings, or numbers);
# [];
# [T] where T is an extended type;
# an object all of whose values are extended types;
# a union type: ["+", "null", T] where T is an extended type not of the same form;
# "JSON" signifying that no other extended-type value is applicable.

# The extended-type of a JSON entity is defined recursively.
# The extended-type of a scalar value is its jq type ("null", "boolean", "string", "number").
# The extended-type of [] is [].
# The extended-type of a non-empty array of values all of which have the
# same extended-type, T, is [T].
# The extended-type of an object is an object with the same keys, but the
#     values of which are the extended-types of the corresponding values.
# The extended-type of an array with values of different extended-types is defined
#     using typeUnion(a;b) as defined below.

# typeUnion(a;b) returns the least extended-type value that subsumes both a and b.
# For example:
#  typeUnion("number"; "string") yields "scalar";
#  typeUnion({"a": "number"}; {"b": "string"}) yields {"a": "number", "b": "string"};
#  if $nullable, then typeUnion("null", t) yields t for any valid extended type, t;
#  otherwise, if t is an extended type, then typeUnion("null"; t) yields t if t already includes
#  null, or else ["+", "null", t].

def typeUnion($a; $b):

  def unionType: type == "array" and .[0] == "+" ;
  def nullable: $ARGS.named.nullable;
  def scalarp: . == "boolean" or . == "string" or . == "number" or . == "scalar";

  def addNull:
    if nullable then .
    elif unionType or . == "null" or . == "scalar" or . == "JSON" then .
    else ["+", "null", .]
    end;

  def addNull($x;$y): typeUnion($x;$y) | addNull;

  if   $a == null or $a == $b then $b
  elif $b == null then $a                        # @2021
  elif $a == "null" then $b | addNull
  elif $b == "null" then $a | addNull
  elif ($a|unionType) and ($b|unionType) then addNull($a[2];$b[2])
  elif $a|unionType then addNull($a[2]; $b)
  elif $b|unionType then addNull($a; $b[2])
  elif ($a | scalarp) and ($b | scalarp) then "scalar"
  elif $a == "JSON" or $b == "JSON" then "JSON"
  elif $a == [] then typeUnion($b; $a)           # @2021
  elif $b == [] and ($a|type) == "array" then $a # @2021
  elif ($a|type) == "array" and ($b|type) == "array" then [ typeUnion($a[0]; $b[0]) ]
  elif ($a|type) == "object" and ($b|type) == "object" then
    ((($a|keys) + ($b|keys)) | unique) as $keys
    | reduce $keys[] as $key ( {} ; .[$key] = typeUnion( $a[$key]; $b[$key]) )
  else "JSON"
  end ;

def typeof:
  def typeofArray:
    if length == 0 then []  # @2021
    else [reduce .[] as $item (null; typeUnion(.; $item|typeof))]
    end ;
  def typeofObject:
    reduce keys[] as $key ( . ; .[$key] |= typeof ) ;

  . as $in
  | type
  | if . == "null" then "null"
    elif . == "string" or . == "number" or . == "boolean" then .
    elif . == "object" then $in | typeofObject
    else $in | typeofArray
    end;

def schema(stream):
  reduce stream as $x (null; typeUnion(.; $x|typeof));

# Omit the outermost [] for an array
def schema:
  if type == "array" then schema(.[])
  else typeof
  end ;

## Example programs:
# typeof
# .. | if type == "object" and has("country") then schema else empty end
# schema
# schema(inputs)