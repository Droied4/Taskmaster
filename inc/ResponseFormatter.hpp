#pragma once

#include <string>

class ResponseFormatter {
public:
  ResponseFormatter() = default;
  ~ResponseFormatter() = default;

  void print_header() const;
  void print_help() const;
  void print_response(const std::string &raw_response) const;
};
