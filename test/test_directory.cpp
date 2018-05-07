#include <iostream>
#include <directory.hpp>

#define throw_assert(assertion, message) do {\
    if (!(assertion)) throw std::runtime_error("condition `" #assertion "` failed: " message); \
} while (0);

int main()
{
  try {
    // try opening a non existing folder -- this shall fail
    try {
      open_dir("/nonexisting");

      // the following lines shall not execute
      std::cerr << "ERROR: /nonexisting folder correcly open!\n";
      return EXIT_FAILURE;
    } catch (...) {}

    // this part shall not fail

    auto bin = open_dir("/bin/");

    auto const list = ls(bin);

    throw_assert(!list.empty(), "empty /bin dir");

    auto const shells = matching_items(bin, ".*sh");

    throw_assert(!shells.empty(), "no shell programs in /bin dir");
  } catch (std::exception& e) {
    std::cerr << e.what() << '\n';
    return EXIT_FAILURE;
  }
}
