#ifndef ELOG_HPP
#define ELOG_HPP

#include <map>
#include <string>

namespace elog {

class post
{
  int m_id;
  std::map<std::string, std::string> m_meta;
  std::string m_message;

 public:

  explicit
  post(int mid)
  : m_id(mid)
  {}

  int id() const
  {
    return m_id;
  }

  std::map<std::string, std::string> const& metadata() const
  {
    return m_meta;
  }

  std::map<std::string, std::string>& metadata()
  {
    return m_meta;
  }

  std::string const& message() const
  {
    return m_message;
  }

  friend
  post make_post(std::istream& input);
};

inline
bool is_reply(post const& p)
{
  return p.metadata().count("In reply to");
}

inline
int has_reply(post const& p)
{
  auto const it = p.metadata().find("Reply to");
  return it == p.metadata().end() ? 0 : std::stoi(it->second);
}

inline
bool is_draft(post const& p)
{
  return p.metadata().count("Draft");
}

post make_post(std::istream& input);

} // ns elog

#endif
