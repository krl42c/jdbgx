# /Users/karol/Library/Java/JavaVirtualMachines/corretto-17.0.8/Contents/Home/include

clang++ \
   jdbg_agent.cc \
   -I/Users/karol/Library/Java/JavaVirtualMachines/corretto-17.0.8/Contents/Home/include\
   -shared -undefined dynamic_lookup -o jdbg_agent.so\
   -Wall -Wextra -Wpedantic\
   -std=c++17
