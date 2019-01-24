# Type Erasure for Unopinionated Interfaces in C++

This code is based on Peter Goldsborough's blog "Type Erasure for Unopinionated Interfaces in C++":  
https://www.goldsborough.me/cpp/2018/05/22/00-32-43-type_erasure_for_unopinionated_interfaces_in_c++/  
My changes:  
 - replaced ad hoc `Any` with `std::any`  
 - used C++17 fold expression to simplify `detail::collect_any_vector()`   
 - implemented classes passed to `Office::work()`  
 - moved everything into a single file for simplicity  
 - renamed a couple of classes and their members  

For detailed explanation of the code, look at the comments in the only source file, `src/main.cpp`.   

The code is complete and buildable with a C++17 compiler (tested with VS2017, GCC 8.2, and Clang 7).
Builds clean even with all cppbestpractices.com recommended warnings:
```
g++ -std=c++17 -Wall -Wextra -Wshadow -Wnon-virtual-dtor -Wold-style-cast -Wcast-align -Wunused -Woverloaded-virtual -Wpedantic -Wsign-conversion -Wmisleading-indentation -Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wnull-dereference -Wuseless-cast -Wdouble-promotion -Wformat=2 src/main.cpp
```

The code prints:  
```
Alice is working on recipe with 3 ingredients: flour, eggs, milk
Peter is working on keyboard, monitor, and coffee
```  
  
