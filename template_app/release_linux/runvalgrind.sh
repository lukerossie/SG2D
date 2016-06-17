valgrind --tool=memcheck --leak-check=yes --track-origins=yes --show-reachable=yes --num-callers=20 --track-fds=yes ./main 2> valgrind.txt

