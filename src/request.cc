// Copyright 2019 Roman Perepelitsa.
//
// This file is part of GitStatus.
//
// GitStatus is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// GitStatus is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with GitStatus. If not, see <https://www.gnu.org/licenses/>.

#include "request.h"

#include <unistd.h>

#include <algorithm>
#include <cstdlib>
#include <iostream>

#include "check.h"
#include "logging.h"
#include "print.h"
#include "serialization.h"

namespace gitstatus {

namespace {

Request ParseRequest(const std::string& s) {
  Request res;
  auto begin = s.begin(), end = s.end(), sep = std::find(begin, end, kFieldSep);
  VERIFY(sep != end) << "Malformed request: " << s;
  res.id.assign(begin, sep);

  begin = sep + 1;
  if (*begin == ':') {
    res.from_dotgit = true;
    ++begin;
  }
  sep = std::find(begin, end, kFieldSep);
  res.dir.assign(begin, sep);
  if (sep == end) return res;

  begin = sep + 1;
  VERIFY(begin + 1 == end && (*begin == '0' || *begin == '1')) << "Malformed request: " << s;
  res.diff = *begin == '0';
  return res;
}

}  // namespace

std::ostream& operator<<(std::ostream& strm, const Request& req) {
  strm << Print(req.id) << " for " << Print(req.dir);
  if (req.from_dotgit) strm << " [from-dotgit]";
  if (!req.diff) strm << " [no-diff]";
  return strm;
}

RequestReader::RequestReader(int fd) : fd_(fd) {}

bool RequestReader::ReadRequest(Request& req) {
  char buf[256];
  int n;
  VERIFY((n = read(fd_, buf, sizeof(buf))) >= 0) << Errno();
  LOG(INFO) << "read(" << n << ") = '" << buf << "'";
  if (n == 0) {
    eof_ = true;
    return false;
  }
  read_.insert(read_.end(), buf, buf + n);
  int eol = std::find(buf, buf + n, kMsgSep) - buf;
  if (eol != n) {
    std::string msg(read_.begin(), read_.end() - (n - eol));
    read_.erase(read_.begin(), read_.begin() + msg.size() + 1);
    req = ParseRequest(msg);
    return true;
  }
  return false;
}

}  // namespace gitstatus
