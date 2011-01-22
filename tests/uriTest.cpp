#include "urilite.h"
#include <gtest/gtest.h>
#include <sstream>

using namespace urilite;

TEST(uriTest, testEncode)
{
  EXPECT_EQ("abcABC123-_.~%21%28%29%26%3d%20",  uri::encode("abcABC123-_.~!()&= ")) << "RFC3986 URL encoded";
  EXPECT_EQ("abcABC123-_.~%21%28%29%26%3d+",  uri::encode2("abcABC123-_.~!()&= ")) << "RFC3986 URL encoded. Space should be escaped to +";
  EXPECT_EQ("abcABC123-_.~!()%26%3d%20",  uri::encodeURIComponent("abcABC123-_.~!()&= ")) << "RFC2396 URL encoded";
  EXPECT_EQ("abcABC123-_.~!()%26%3d+",  uri::encodeURIComponent2("abcABC123-_.~!()&= ")) << "RFC2396 URL encoded. Space should be escaped to +";

  EXPECT_EQ("abcABC123-_.~!()&= ", uri::decode("abcABC123-_.~%21%28%29%26%3d%20")) << "decode correctly";
  EXPECT_EQ("abcABC123-_.~!()&= ", uri::decode2("abcABC123-_.~%21%28%29%26%3d%20")) << "decode correctly";

  EXPECT_EQ("abcABC123-_.~!()&=+", uri::decode("abcABC123-_.~%21%28%29%26%3d+")) << "decode correctly";
  EXPECT_EQ("abcABC123-_.~!()&= ", uri::decode2("abcABC123-_.~%21%28%29%26%3d+")) << "decode correctly";

  EXPECT_EQ("abcABC123-_.~!()&= ", uri::decode("abcABC123-_.~!()%26%3d%20")) << "decode correctly";
  EXPECT_EQ("abcABC123-_.~!()&= ", uri::decode2("abcABC123-_.~!()%26%3d%20")) << "decode correctly";

  EXPECT_EQ("abcABC123-_.~!()&=+", uri::decode("abcABC123-_.~!()%26%3d+")) << "decode correctly";
  EXPECT_EQ("abcABC123-_.~!()&= ", uri::decode2("abcABC123-_.~!()%26%3d+")) << "decode correctly";
}

TEST(uriTest, testParseBasic)
{
  uri u = uri::parse("http://www.example.org/path/path1?qkey=qval#frag");
  EXPECT_EQ("http", u.scheme());
  EXPECT_FALSE(u.secure());
  EXPECT_EQ("www.example.org", u.host());
  EXPECT_EQ(80, u.port());
  EXPECT_EQ("/path/path1", u.path());
  EXPECT_EQ("http://www.example.org/path/path1?qkey=qval#frag", u.str());
  u.append_query("new_key", "va l");
  EXPECT_EQ("new_key=va%20l&qkey=qval", u.query_string());
  EXPECT_EQ("frag", u.fragment());
  EXPECT_EQ("http://www.example.org/path/path1?new_key=va%20l&qkey=qval#frag", u.str());
  EXPECT_EQ("/path/path1?new_key=va%20l&qkey=qval", u.relstr());

  std::ostringstream os;
  os << u;
  EXPECT_EQ("http://www.example.org/path/path1?new_key=va%20l&qkey=qval#frag", os.str());

  const uri::query_params& params = u.query();
  uri::query_params::const_iterator iter = params.begin();
  EXPECT_EQ("new_key", iter->first);
  EXPECT_EQ("va l", iter->second);
  ++iter;
  EXPECT_EQ("qkey", iter->first);
  EXPECT_EQ("qval", iter->second);
  ++iter;
  EXPECT_TRUE(iter == params.end());
}

TEST(uriTest, testScheme) {
  uri u = uri::parse("http://www.example.org:80/path/path1?qkey=qval#frag");
  EXPECT_EQ("http", u.scheme());
  uri u2 = uri::parse("https://www.example.org:80/path/path1?qkey=qval#frag");
  EXPECT_EQ("https", u2.scheme());

  ASSERT_ANY_THROW(uri::parse("http://")) << "too short to parse";
  ASSERT_ANY_THROW(uri::parse("file://foo/")) << "not http(s)";
  ASSERT_ANY_THROW(uri::parse("www.example.org:80/path/path1?qkey=qval#frag")) << "scheme omitted";
}

TEST(uriTest, testHost) {
  uri u = uri::parse("http://www.example.org:80/path/path1?qkey=qval#frag");
  EXPECT_EQ("www.example.org", u.host());

  uri u2 = uri::parse("http://www.example.org");
  EXPECT_EQ("www.example.org", u2.host());

  uri u3 = uri::parse("http://localhost/");
  EXPECT_EQ("localhost", u3.host());

  ASSERT_ANY_THROW( uri::parse("http://:80/path") ) << "no host";
  ASSERT_ANY_THROW( uri::parse("http:///path") ) << "no host";
  ASSERT_ANY_THROW( uri::parse("http://?query") ) << "no host";
  ASSERT_ANY_THROW( uri::parse("http://#frag") ) << "no host";
}

TEST(uriTest, testPath) {
  uri u = uri::parse("http://www.example.org:80/path?qkey=qval#frag");
  EXPECT_EQ("/path", u.path());

  uri u2 = uri::parse("http://www.example.org?qkey=qval#frag");
  EXPECT_EQ("/", u2.path());

  uri u3 = uri::parse("http://www.example.org/path");
  EXPECT_EQ("/path", u3.path());
  
  uri u4 = uri::parse("http://localhost/");
  EXPECT_EQ("/", u4.path());

  uri u5 = uri::parse("http://localhost");
  EXPECT_EQ("/", u5.path());

  uri u6 = uri::parse("http://www.example.org#frag");
  EXPECT_EQ("/", u6.path());
}

TEST(uriTest, testPort) {
  // normal with 80 port
  uri u2 = uri::parse("http://www.example.org:80/path/path1?qkey=qval#frag");
  EXPECT_FALSE(u2.secure());
  EXPECT_EQ(80, u2.port());
  EXPECT_EQ("http://www.example.org/path/path1?qkey=qval#frag", u2.str());

  // secure without 443 port
  uri u3 = uri::parse("https://www.example.org/path/path1?qkey=qval#frag");
  EXPECT_TRUE(u3.secure());
  EXPECT_EQ(443, u3.port());
  EXPECT_EQ("https://www.example.org/path/path1?qkey=qval#frag", u3.str());

  // secure with 443 port
  uri u4 = uri::parse("https://www.example.org:443/path/path1?qkey=qval#frag");
  EXPECT_TRUE(u4.secure());
  EXPECT_EQ(443, u4.port());
  EXPECT_EQ("https://www.example.org/path/path1?qkey=qval#frag", u4.str());

  // normal with non-80 port
  uri u5 = uri::parse("http://www.example.org:8080/path/path1?qkey=qval#frag");
  EXPECT_FALSE(u5.secure());
  EXPECT_EQ(8080, u5.port());
  EXPECT_EQ("http://www.example.org:8080/path/path1?qkey=qval#frag", u5.str());

  // secure with non-443 port
  uri u6 = uri::parse("https://www.example.org:8080/path/path1?qkey=qval#frag");
  EXPECT_TRUE(u6.secure());
  EXPECT_EQ(8080, u6.port());
  EXPECT_EQ("https://www.example.org:8080/path/path1?qkey=qval#frag", u6.str());

  // invalid port ( including NaN )
  ASSERT_ANY_THROW( uri::parse("https://www.example.org:aa9/path/path1?qkey=qval#frag") );
}

