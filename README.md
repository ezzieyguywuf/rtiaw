Overview
--------
This is my code as I work through the fabulous [Ray Tracing in a Weekend][1]
book.

I plan to write this in C++, though I'm interested in also trying in rust.

I also don't plan to follow the design patterns used in the book to a T.

Dependencies
-------------

For the C++ code, you'll need:

    - a c++ compiler (I use clang, but any should work)
    - cmake
    - sfml

How To Build and Run
--------------------

For the C++ code:

```sh
mkdir build
cd build
cmake ..
make

# you may have to use backslashes depending on your system
./bin/ray_tracer
```


[1]: https://raytracing.github.io/books/RayTracingInOneWeekend.html
