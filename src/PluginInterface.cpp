#include "PluginInterface.h"

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

    TableBuilder table_builder("OpenTradesReportTable");

    table_builder.SetIdColumn("order");
    table_builder.SetOrderBy("order", "DESC");
    table_builder.EnableAutoSave(false);
    table_builder.EnableRefreshButton(false);
    table_builder.EnableBookmarksButton(false);
    table_builder.EnableExportButton(true);
    table_builder.EnableTotal(true);
    table_builder.SetTotalDataTitle("TOTAL");

    table_builder.AddColumn({"order", "ORDER", 1});
    table_builder.AddColumn({"login", "LOGIN", 2});
    table_builder.AddColumn({"name", "NAME", 3});
    table_builder.AddColumn({"open_time", "OPEN_TIME", 4});
    table_builder.AddColumn({"type", "TYPE", 5});
    table_builder.AddColumn({"symbol", "SYMBOL", 6});
    table_builder.AddColumn({"volume", "VOLUME", 7});
    table_builder.AddColumn({"open_price", "OPEN_PRICE", 8});
    table_builder.AddColumn({"sl", "S / L", 9});
    table_builder.AddColumn({"tp", "T / P", 10});
    table_builder.AddColumn({"storage", "SWAP", 11});
    table_builder.AddColumn({"profit", "AMOUNT", 12});
    table_builder.AddColumn({"comment", "COMMENT", 13});
    table_builder.AddColumn({"currency", "CURRENCY", 14});

    for (const auto& trade : trades_vector) {
        AccountRecord account;

        try {
            server->GetAccountByLogin(trade.login, &account);
        } catch (const std::exception& e) {
            std::cerr << "[OpenTradesReportInterface]: " << e.what() << std::endl;
        }

        total_volume += trade.volume;

        const std::string currency = utils::GetGroupCurrencyByName(groups_vector, account.group);
        double multiplier = 1;

        table_builder.AddRow({
            utils::TruncateDouble(trade.order, 0),
            utils::TruncateDouble(trade.login, 0),
            account.name,
            utils::FormatTimestampToString(trade.open_time),
            trade.cmd == 0 ? "buy" : "sell",
            trade.symbol,
            utils::TruncateDouble(trade.volume, 0),
            utils::TruncateDouble(trade.open_price * multiplier, 2),
            utils::TruncateDouble(trade.sl * multiplier, 2),
            utils::TruncateDouble(trade.tp * multiplier, 2),
            utils::TruncateDouble(trade.storage * multiplier, 2),
            utils::TruncateDouble(trade.profit * multiplier, 2),
            trade.comment,
            "USD"
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