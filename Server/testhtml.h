#ifndef HTML_TEST_H
#define HTML_TEST_H

#include <string>

namespace test_htmls
{
inline static constexpr char test_html[] = \
"HTTP/1.1 200 OK\r\n\
Version: HTTP/1.1\r\n\
Content-Type: text/html; charset=utf-8\r\n\
Content-Length: 59\
\r\n\r\n\
<!DOCTYPE html>\n\
<html>\n\
<body>\n\
<p>Test</p>\n\
</body>\n\
</html>\n";

} // namespace test_htmls


#endif //HTML_TEST_H