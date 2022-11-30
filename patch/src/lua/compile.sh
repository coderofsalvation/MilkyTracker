#!/bin/sh

{
  echo "#ifndef LUA_COMPILED_H"
  echo "#define LUA_COMPILED_H"
  for i in $(find *.lua); do 
    xxd -i "$i"
  done
  echo "#endif"
} > compiled.h
