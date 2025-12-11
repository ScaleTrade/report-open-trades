#include "PluginInterface.hpp"

#include <iomanip>

extern "C" void AboutReport(rapidjson::Value& request,
                            rapidjson::Value& response,
                            rapidjson::Document::AllocatorType& allocator,
                            CServerInterface* server) {
    response.AddMember("version", 1, allocator);
    response.AddMember("name", Value().SetString("Open Trades report", allocator), allocator);
    response.AddMember("description",
    Value().SetString("Summary data on open trades executed by a selected group of traders over a specified day. "
                                 "Includes date, symbol, price, profit, volume, s / l, t / p, commission, swap and account information.",
             allocator), allocator);
    response.AddMember("type", REPORT_DAILY_GROUP_TYPE, allocator);
}

extern "C" void DestroyReport() {}

extern "C" void CreateReport(rapidjson::Value& request,
                             rapidjson::Value& response,
                             rapidjson::Document::AllocatorType& allocator,
                             CServerInterface* server) {
    std::string group_mask;
    int from;
    int to;
    if (request.HasMember("group") && request["group"].IsString()) {
        group_mask = request["group"].GetString();
    }
    if (request.HasMember("from") && request["from"].IsNumber()) {
        from = request["from"].GetInt();
    }
    if (request.HasMember("to") && request["to"].IsNumber()) {
        to = request["to"].GetInt();
    }

    double total_volume;
    std::vector<TradeRecord> trades_vector;
    std::vector<GroupRecord> groups_vector;

    try {
        server->GetOpenTradesByGroup(group_mask, from, to, &trades_vector);
        server->GetAllGroups(&groups_vector);
    } catch (const std::exception& e) {
        std::cerr << "[OpenTradesReportInterface]: " << e.what() << std::endl;
    }

    // Лямбда для поиска валюты аккаунта по группе
    auto get_group_currency = [&](const std::string& group_name) -> std::string {
        for (const auto& group : groups_vector) {
            if (group.group == group_name) {
                return group.currency;
            }
        }
        return "N/A"; // группа не найдена - валюта не определена
    };

    // Лямбда подготавливающая значения double для вставки в AST (округление до 2-х знаков)
    auto format_double_for_AST = [](double value) -> std::string {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << value;
        return oss.str();
    };

    TableBuilder table_builder("OpenTradesReportTable");

    table_builder.SetIdColumn("order");
    table_builder.SetOrderBy("order", "DESC");
    table_builder.EnableRefreshButton(false);
    table_builder.EnableBookmarksButton(false);
    table_builder.EnableExportButton(true);
    table_builder.EnableTotal(true);
    table_builder.SetTotalDataTitle("TOTAL");

    table_builder.AddColumn({"order", "ORDER"});
    table_builder.AddColumn({"login", "LOGIN"});
    table_builder.AddColumn({"name", "NAME"});
    table_builder.AddColumn({"open_time", "OPEN_TIME"});
    table_builder.AddColumn({"type", "TYPE"});
    table_builder.AddColumn({"symbol", "SYMBOL"});
    table_builder.AddColumn({"volume", "VOLUME"});
    table_builder.AddColumn({"open_price", "OPEN_PRICE"});
    table_builder.AddColumn({"sl", "S / L"});
    table_builder.AddColumn({"tp", "T / P"});
    table_builder.AddColumn({"storage", "SWAP"});
    table_builder.AddColumn({"profit", "AMOUNT"});
    table_builder.AddColumn({"comment", "COMMENT"});

    for (const auto& trade : trades_vector) {
        AccountRecord account;

        try {
            server->GetAccountByLogin(trade.login, &account);
        } catch (const std::exception& e) {
            std::cerr << "[OpenTradesReportInterface]: " << e.what() << std::endl;
        }

        total_volume += trade.volume;

        const std::string currency = get_group_currency(account.group);
        double multiplier = 1;

        table_builder.AddRow({
            {"order", std::to_string(trade.order)},
            {"login", std::to_string(trade.login)},
            {"name", account.name},
            {"open_time", utils::FormatTimestampToString(trade.open_time)},
            {"type", trade.cmd == 0 ? "buy" : "sell"},
            {"symbol", trade.symbol},
            {"volume", format_double_for_AST(trade.volume)},
            {"open_price", format_double_for_AST(trade.open_price * multiplier)},
            {"sl", std::to_string(trade.sl * multiplier)},
            {"tp", std::to_string(trade.tp * multiplier)},
            {"storage", std::to_string(trade.storage * multiplier)},
            {"profit", format_double_for_AST(trade.profit * multiplier)},
            {"comment", trade.comment}
        });
    }

    // Total row
    JSONArray totals_array;
    totals_array.emplace_back(JSONObject{
        {"volume", total_volume}
    });

    table_builder.SetTotalData(totals_array);

    const JSONObject table_props = table_builder.CreateTableProps();
    const Node table_node = Table({}, table_props);

    // Total report
    const Node report = div({
        h1({text("Open Trades Report")}),
        table_node
    });

    utils::CreateUI(report, response, allocator);
}