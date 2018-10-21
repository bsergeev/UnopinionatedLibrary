# Type Erasure for Unopinionated Interfaces in C++

This code is based on Peter Goldsborough's blog "Type Erasure for Unopinionated Interfaces in C++":  
https://www.goldsborough.me/cpp/2018/05/22/00-32-43-type_erasure_for_unopinionated_interfaces_in_c++/  
The code is complete and buildable with a C++17 compiler (tested with VS2017 and Clang 7).  
When it runs, it prints:  
```
Alice is working on recipe with 3 ingredients: flour, eggs, milk
Peter is working on keyboard, monitor, and coffee
```  
  
My changes:  
 - replaces `Any` with `std::any`  
 - used C++17 fold expression to simplify `detail::collect_any_vector()`   
 - implemented classes passed to `Office::work()`  
 - moved everything into a single file for simplicity  
 - renamed a couple of classes and their members  