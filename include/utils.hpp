// #include <bits/stdc++.h>
// #include <limits>
#include <iostream>
#include <stdexcept>

class CustomExcept : public std::exception {
public:
   const char* what() const noexcept override {
      return "This is my custom exception ;D";
   }
};
