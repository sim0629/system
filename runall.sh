#!/bin/bash

for i in {01..16}
do
    make test$i
done
