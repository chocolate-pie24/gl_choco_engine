#!/bin/bash

valgrind --tool=memcheck --track-origins=yes --leak-check=full --show-leak-kinds=all --errors-for-leak-kinds=all --error-exitcode=1 ./bin/gl_choco_engine
