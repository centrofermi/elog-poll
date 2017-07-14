#include <iostream>
#include <vector>
#include <regex>
#include "elog.hpp"

std::string compile(std::string const& input, elog::post const& post)
{
  std::regex const placeholder_regex {"\\%\\[[ a-zA-Z0-9]*\\]"};

  auto begin = std::sregex_iterator(
      std::begin(input)
    , std::end(input)
    , placeholder_regex
  );

  auto const end = std::sregex_iterator();
  auto const nmatches = std::distance(begin, end);

  std::vector<std::sregex_iterator> v(nmatches);

  for (auto i = 0; i != nmatches; ++i) {
    v[i] = begin++;
  }

  std::string out = input;
  for (auto it = v.crbegin(); it != v.crend(); ++it) {
    auto const& x = *it;
    auto const match = x->str();
    std::string const token {match.begin() + 2, match.end() - 1};

    if (token == "id") {
      out.replace(
          x->position()
        , x->length()
        , std::to_string(post.id())
      );
    } else if (token == "message") {
      out.replace(
          x->position()
        , x->length()
        , post.message()
      );
    } else {
      auto const it = post.metadata().find(token);

      if (it != post.metadata().end()) {
        out.replace(
            x->position()
          , x->length()
          , it->second
        );
      }
    }
  }

  return out;
}

int main(int argc, char* argv[])
{
  try {
    if (argc > 1) {
      elog::post const post = elog::make_post(std::cin);

      auto const output = compile(
          argv[1]
        , post
      );

      std::cout << output << '\n';
    } else {
      std::cin >> std::noskipws;
      std::copy(
          std::istream_iterator<char>(std::cin)
        , std::istream_iterator<char>()
        , std::ostream_iterator<char>(std::cout)
      );
    }
  } catch (std::exception const& e) {
    std::cerr << e.what() << '\n';
    return EXIT_FAILURE;
  } catch (...) {
    std::cerr << "Unknown exception\n";
    return EXIT_FAILURE;
  }
}
