#pragma once

#include <ctime>
#include <iomanip>
#include <sstream>
#include "ast/Ast.hpp"
#include <rapidjson/document.h>
#include "Structures.hpp"

using namespace ast;

namespace utils {
    void CreateUI(const ast::Node& node,
              rapidjson::Value& response,
              rapidjson::Document::AllocatorType& allocator);

    std::string FormatTimestampToString(const time_t timestamp);
}