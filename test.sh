#!/bin/bash

parser=out/parser

for file in $@; do
  mkdir -p out/$(dirname $file)
  result_file=out/$file-result
  $parser $file &> $result_file
  python compare.py $result_file testout-true/$file
done
