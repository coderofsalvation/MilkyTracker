#!/bin/sh
for i in $(find *.lua); do 
  xxd -i "$i"
done > compiled.h
