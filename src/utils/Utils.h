#pragma once

#include <cmath>
#include <ctime>
#include <iomanip>
#include <set>
#include <sstream>

#include "ReportServerInterface.h"
#include "ast/Ast.hpp"
#include "rapidjson/document.h"

using namespace ast;

namespace utils {
    void CreateUI(const ast::Node&                    node,
                  rapidjson::Value&                   response,
                  rapidjson::Document::AllocatorType& allocator);

    std::string FormatTimestampToString(const time_t&      timestamp,
                                        const std::string& format = "%Y.%m.%d %H:%M:%S");

    double TruncateDouble(const double& value, const int& digits);

    std::string GetGroupCurrencyByName(const std::vector<ReportGroupRecord>& group_vector,
                                       const std::string&                    group_name);

    std::string ConvertCmdToString(const int cmd);

    std::string Trim(const std::string& str);

    std::set<std::string> SplitToSet(const std::string& str);

    std::string Trim(const std::string& str) {
        const auto begin = str.find_first_not_of(" \t");
        if (begin == std::string::npos)
            return "";
        const auto end = str.find_last_not_of(" \t");
        return str.substr(begin, end - begin + 1);
    }

    std::set<std::string> SplitToSet(const std::string& str) {
        std::set<std::string> out;
        std::stringstream     ss(str);
        std::string           item;
        while (std::getline(ss, item, ',')) {
            const std::string trimmed = Trim(item);
            if (!trimmed.empty())
                out.insert(trimmed);
        }
        return out;
    }
} // namespace utils