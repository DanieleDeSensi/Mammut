#!/bin/bash

for t in *.cpp; do
	cd archs && tar -xf repara.tar.gz
	cd .. 
	./$(basename "$t" .cpp)
    exitvalue=$?
	cd archs && rm -rf repara # Remove folder after test since it becomes dirty
	cd ..
    if [ $exitvalue -ne 0 ]; then
        break
    fi
done

exit $exitvalue
