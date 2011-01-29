/*
The MIT License

Copyright (c) 2011 lyo.kato@gmail.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#ifndef _URILITE_H_
#define _URILITE_H_
#include <string>
#include <sstream>
#include <iomanip>
#include <map>
#include <stdexcept>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/utility.hpp>

namespace urilite {

class PLUS : public boost::noncopyable {
 public:
  static const std::string space_encoded() { return "+"; };
  static const char plus_decoded() { return ' '; };
};

class NO_PLUS : public boost::noncopyable {
 public:
  static const std::string space_encoded() { return "%20"; };
  static const char plus_decoded() { return '+'; };
};

class RFC2396 : public boost::noncopyable {
public:
  static bool match(const char c) {
    // unreserved?
    if ((c >= 'a' && c <= 'z')
     || (c >= 'A' && c <= 'Z')
     || (c >= '0' && c <= '9')
     || c == '-'  || c == '_'  || c == '.' || c == '~'
     || c == '!'  || c == '\'' || c == '(' || c == ')')
      return false;
    else
      return true;
  }
};

class RFC3986 : public boost::noncopyable {
public:
  static bool match(const char c) {
    // unreserved?
    if ((c >= 'a' && c <= 'z')
     || (c >= 'A' && c <= 'Z')
     || (c >= '0' && c <= '9')
     || c == '-' || c == '_' || c == '.' || c == '~')
      return false;
    else
      return true;
  }
};

template<typename T = RFC3986, typename U = NO_PLUS>
class encoder : public boost::noncopyable {
public:
    static std::string encode(const std::string& s) {
        std::ostringstream oss;
        for (int i=0; i < s.size(); ++i) {
            char c = s[i];
            if (!T::match(c)) {
                oss << c;
            } else if (c == ' ') {
                oss << U::space_encoded();
            } else {
                oss << '%' << std::hex << std::setw(2) << std::setfill('0') << (c & 0xff) << std::dec;
            }
        }
        return oss.str();
    }

    static std::string decode(const std::string& s) {
        std::ostringstream oss;
        for (int i = 0; i < s.size(); ++i) {
            char c = s[i];
            if (c == '+') {
                oss << U::plus_decoded();
            } else if (c == '%') {
                int d;
                std::istringstream iss(s.substr(i+1, 2));
                iss >> std::hex >> d;
                oss << static_cast<char>(d);
                i += 2;
            } else {
                oss << c;
            }
        }
        return oss.str();
    }
}; // end of class


class uri {

  friend std::ostream& operator<<(std::ostream& os, const uri& u);

 public:

  typedef std::multimap<std::string, std::string> query_params;
  typedef std::pair<std::string, std::string>     query_param;

  static std::string encode(const std::string& s) {
    return encoder<RFC3986, NO_PLUS>::encode(s);
  }

  static std::string decode(const std::string& s) {
    return encoder<RFC3986, NO_PLUS>::decode(s);
  }

  static std::string encode2(const std::string& s) {
    return encoder<RFC3986, PLUS>::encode(s);
  }

  static std::string decode2(const std::string& s) {
    return encoder<RFC3986, PLUS>::decode(s);
  }

  static std::string encodeURIComponent(const std::string& s) {
    return encoder<RFC2396, NO_PLUS>::encode(s);
  }

  static std::string encodeURIComponent2(const std::string& s) {
    return encoder<RFC2396, PLUS>::encode(s);
  }

  // support absoluteURI, net_path only
  static uri parse(const std::string& s)
  {
    bool secure = false;

    const char* c = s.c_str();
    std::ostringstream os;

    if (s.size() < 9)
      throw std::invalid_argument("uri is too short");

    // check scheme
    if (!(*c == 'h' || *c == 'H'))
      throw std::invalid_argument("invalid scheme");
    ++c;
    if (!(*c == 't' || *c == 'T'))
      throw std::invalid_argument("invalid scheme");
    ++c;
    if (!(*c == 't' || *c == 'T'))
      throw std::invalid_argument("invalid scheme");
    ++c;
    if (!(*c == 'p' || *c == 'P'))
      throw std::invalid_argument("invalid scheme");

    ++c;
    if (*c == 's' || *c == 'S') {
      secure = true;
      ++c;
    }

    if (*c != ':')
      throw std::invalid_argument("invalid scheme");
    ++c;
    if (*c != '/')
      throw std::invalid_argument("invalid scheme");
    ++c;
    if (*c != '/')
      throw std::invalid_argument("invalid scheme");

    ++c;
    // check host
    while (!(*c ==':' || *c == '/' || *c == '?' || *c == '#' || *c == '\0')) {
      // TODO validate
      os << *c;
      ++c;
    }

    const std::string host(os.str());
    if (host.empty())
      throw std::invalid_argument("host not found");
    os.str("");
    os.clear(std::stringstream::goodbit);

    unsigned short port;
    // found port
    if (*c != ':') {
      port = secure ? 443 : 80;
    } else {
      ++c;
      while (!(*c == '/' || *c == '?' || *c == '#' || *c == '\0')) {
        if (!('0' <= *c && *c <= '9'))
          throw std::invalid_argument("invalid port");
        os << *c;
        ++c;
      }
      port = boost::lexical_cast<unsigned short>(os.str());
      os.str("");
      os.clear(std::stringstream::goodbit);
    }

    os << "/";
    // found path
    if (*c == '/') {
      ++c;
      while (!(*c == '?' || *c == '#' || *c == '\0')) {
        // TODO validate
        os << *c;
        ++c;
      }
    }

    const std::string path(os.str());
    os.str("");
    os.clear(std::stringstream::goodbit);

    // found querystring
    if (*c == '?') {
      ++c;
      while (!(*c == '#' || *c == '\0')) {
        // TODO validate
        os << *c;
        ++c;
      }
    }
    const std::string query(os.str());
    os.str("");
    os.clear(std::stringstream::goodbit);

    // found fragment
    if (*c == '#') {
      ++c;
      while (*c != '\0') {
        // TODO validate
        os << *c;
        ++c;
      }
    }
    const std::string fragment(os.str());
    os.str("");
    os.clear(std::stringstream::goodbit);

    uri u(secure, host, port, path, fragment);

    if (!query.empty()) {

      const char *q = query.c_str();

      std::string qkey;
      std::string qval;

      while (true) {

        while (*q != '=' && *q != '\0') {
          os << *q;
          ++q;
        }
        if (*q == '\0')
          throw std::invalid_argument("invalid query");
        qkey = uri::decode(os.str());
        if (qkey.empty())
          throw std::invalid_argument("invalid query: key is empty");
        os.str("");
        os.clear(std::stringstream::goodbit);
        ++q;
        while (*q != '&' && *q != '\0') {
          os << *q;
          ++q;
        }
        qval = uri::decode(os.str());
        os.str("");
        os.clear(std::stringstream::goodbit);
        u.append_query(qkey, qval);
        if (*q == '&') {
          ++q;
          continue;
        } else {
          break;
        }
      }
    }
    return u;
  }

  void append_query(const std::string& key, const std::string& value) {
    query_.insert(std::make_pair(key, value));
  }

  const bool secure() const {
    return secure_;
  }

  const std::string scheme() const {
    return secure_ ? "https" : "http";
  }

  const std::string host() const {
    return host_;
  }

  const unsigned short port() const {
    return port_;
  }

  const std::string path() const {
    return path_;
  }

  const std::string fragment() const {
    return fragment_;
  }

  const query_params& query() const {
    return query_;
  }

  const std::string query_string() const {
    std::vector<std::string> v;
    BOOST_FOREACH(query_param param, query_) {
      v.push_back((
        boost::format("%1%=%2%")
          % uri::encode(param.first)
          % uri::encode(param.second)).str());
    }
    std::sort(v.begin(), v.end());
    return boost::algorithm::join(v, "&");
  }

  const std::string str() const {
    std::ostringstream os;
    os << (secure_ ? "https://" : "http://");
    os << host_;
    if (!
         ((!secure_ && port_ ==  80)
        ||( secure_ && port_ == 443)) )
      os << ":" << port_;
    os << path_;
    if (!query_.empty())
      os << "?" << this->query_string();
    if (!fragment_.empty())
      os << "#" << fragment_;
    return os.str();
  }

  const std::string authority() const {
    std::ostringstream os;
    os << host_;
    if (!
         ((!secure_ && port_ ==  80)
        ||( secure_ && port_ == 443)) )
      os << ":" << port_;
    return os.str();
  }

  const std::string relative() const {
    std::ostringstream os;
    os << path_;
    if (!query_.empty())
      os << "?" << this->query_string();
    return os.str();
  }

 private:
  uri(bool               secure, 
      const std::string& host, 
      unsigned short     port,
      const std::string& path,
      const std::string& fragment)
    : secure_(secure)
    , host_(host)
    , port_(port)
    , path_(path)
    , fragment_(fragment) { }
  bool           secure_;
  std::string    host_;
  unsigned short port_;
  std::string    path_;
  std::string    fragment_;
  query_params   query_;
}; // end of class

std::ostream&
operator<<(std::ostream& os, const uri& u)
{
  os << u.str();
  return os;
}

}  // end of namespace

#endif
