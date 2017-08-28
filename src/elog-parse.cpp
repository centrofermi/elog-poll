#include <iostream>
#include <fstream>
#include <vector>
#include <regex>
#include <cstring>
#include "elog.hpp"

std::string compile(std::string const& input, elog::post const& post)
{
  std::regex const placeholder_regex {"\\%\\[[ a-zA-Z0-9()]*\\]"};

  auto begin = std::sregex_iterator(
      std::begin(input)
    , std::end(input)
    , placeholder_regex
  );

  auto const end = std::sregex_iterator();
  auto const nmatches = std::distance(begin, end);

  std::vector<std::sregex_iterator> v(nmatches);

  for (auto& x : v) {
    x = begin++;
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
      std::ifstream file;
      std::istream& input
        = argc == 2
        ? static_cast<std::istream&>(std::cin)
        : static_cast<std::istream&>((file.open(argv[2]), file));

      if (std::strcmp(argv[1], "-h") == 0) {
        std::cout
          << "Elog post reader with text manipulation capabilities\n\n"
             "Usage:\n"
          << argv[0] << "[-h|-c|-r|text] [filename]\n"
          << "   -h: show this help and exit\n"
             "   -c: check the validity of the post and exit\n"
             "   -r: check that the message is a reply and exit\n"
             " text: manipulate the provided text replacing any occurrence"
             " of %[attr] with the\n"
             "       corresponding elog message attribute value\n\n"
             "If a file name is provided messages are read from it"
             " otherwise standard input\n"
             "is used\n";
      } else if (std::strcmp(argv[1], "-c") == 0) {
        try {
          elog::make_post(input);
          return EXIT_SUCCESS;
        } catch (...) {
          return EXIT_FAILURE;
        }
      } else if (std::strcmp(argv[1], "-r") == 0) {
        return is_reply(elog::make_post(input)) ? EXIT_SUCCESS : EXIT_FAILURE;
      } else {
        elog::post const post = elog::make_post(input);

        auto const output = compile(
            argv[1]
          , post
        );

        std::cout << output << '\n';
      }
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
