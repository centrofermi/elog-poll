#include "elog.hpp"
#include <iterator>

namespace elog {

post make_post(std::istream& input)
{
  std::string line;
  std::getline(input, line);

  std::string::size_type const pos = line.find(':');
  std::string const key = line.substr(0, pos);

  if (key != "$@MID@$") {
    throw std::runtime_error("Input does not contain a message");
  }

  int const id = std::stoi(line.substr(pos + 1));

  post p(id);

  while (input && input.peek() != EOF) {
    std::getline(input, line);

    std::string::size_type const pos = line.find(':');

    if (pos == std::string::npos && line[0] == '=') {
      break;
    }

    std::string const key = line.substr(0, pos);

    p.metadata()[key] = line.substr(pos + 2);
  }

  std::istream_iterator<char> iter(input);

  input >> std::noskipws;

  std::copy(
      iter
    , std::istream_iterator<char>()
    , std::back_inserter(p.m_message)
  );

  return p;
}

} // ns elog

