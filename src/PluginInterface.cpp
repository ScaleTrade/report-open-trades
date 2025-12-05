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
    double total_volume;

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

    std::vector<TradeRecord> trades_vector;

    try {
        server->GetOpenTradesByGroup(group_mask, from, to, &trades_vector);
    } catch (const std::exception& e) {
        std::cerr << "[OpenTradesReportInterface]: " << e.what() << std::endl;
    }

    // Лямбда подготавливающая значения double для вставки в AST (округление до 2-х знаков)
    auto format_double_for_AST = [](double value) -> std::string {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << value;
        return oss.str();
    };

    // v.1
    // auto create_table = [&](const std::vector<TradeRecord>& trades) -> Node {
    //     std::vector<Node> thead_rows;
    //     std::vector<Node> tbody_rows;
    //     std::vector<Node> tfoot_rows;
    //
    //     // Thead
    //     thead_rows.push_back(tr({
    //         th({div({text("Order")})}),
    //         th({div({text("Login")})}),
    //         th({div({text("Name")})}),
    //         th({div({text("Open Time")})}),
    //         th({div({text("Type")})}),
    //         th({div({text("Symbol")})}),
    //         th({div({text("Volume")})}),
    //         th({div({text("Open Price")})}),
    //         th({div({text("S / L")})}),
    //         th({div({text("T / P")})}),
    //         th({div({text("Swap")})}),
    //         th({div({text("Profit")})}),
    //         th({div({text("Comment")})}),
    //     }));
    //
    //     // Tbody
    //     for (const auto& trade : trades_vector) {
    //         AccountRecord account;
    //
    //         server->GetAccountByLogin(trade.login, &account);
    //
    //         total_volume += trade.volume;
    //
    //         tbody_rows.push_back(tr({
    //             td({div({text(std::to_string(trade.order))})}),
    //             td({div({text(std::to_string(trade.login))})}),
    //             td({div({text(account.name)})}),
    //             td({div({text(utils::FormatTimestampToString(trade.open_time))})}),
    //             td({div({text(trade.cmd == 0 ? "buy" : "sell")})}),
    //             td({div({text(trade.symbol)})}),
    //             td({div({text(format_double_for_AST(trade.volume))})}),
    //             td({div({text(format_double_for_AST(trade.open_price))})}),
    //             td({div({text(std::to_string(trade.sl))})}),
    //             td({div({text(std::to_string(trade.tp))})}),
    //             td({div({text(std::to_string(trade.storage))})}),
    //             td({div({text(format_double_for_AST(trade.profit))})}),
    //             td({div({text(trade.comment)})}),
    //         }));
    //     }
    //
    //     // Tfoot
    //     tfoot_rows.push_back(tr({
    //         td({div({text("TOTAL:")})}),
    //         td({div({text("")})}),
    //         td({div({text("")})}),
    //         td({div({text("")})}),
    //         td({div({text("")})}),
    //         td({div({text("")})}),
    //         td({div({text("")})}),
    //         td({div({text("")})}),
    //         td({div({text("")})}),
    //         td({div({text("")})}),
    //         td({div({text("")})}),
    //         td({div({text("")})}),
    //         td({div({text("")})})
    //     }));
    //
    //     tfoot_rows.push_back(tr({
    //         td({div({text("")})}),
    //         td({div({text("")})}),
    //         td({div({text("")})}),
    //         td({div({text("")})}),
    //         td({div({text("")})}),
    //         td({div({text("")})}),
    //         td({div({text(format_double_for_AST(total_volume))})}),
    //         td({div({text("")})}),
    //         td({div({text("")})}),
    //         td({div({text("")})}),
    //         td({div({text("")})}),
    //         td({div({text("")})}),
    //         td({div({text("")})}),
    //     }));
    //
    //     return table({
    //         thead(thead_rows),
    //         tbody(tbody_rows),
    //         tfoot(tfoot_rows),
    //     }, props({{"className", "table"}}));
    // };

    // v.2
    TableBuilder table_builder("OpenTradesReportTable");

    table_builder.SetIdColumn("order");
    table_builder.SetOrderBy("order", "DESC");
    table_builder.EnableRefreshButton(false);
    table_builder.EnableBookmarksButton(false);
    table_builder.EnableExportButton(true);

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

        table_builder.AddRow({
            {"order", std::to_string(trade.order)},
            {"login", std::to_string(trade.login)},
            {"name", account.name},
            {"open_time", utils::FormatTimestampToString(trade.open_time)},
            {"type", trade.cmd == 0 ? "buy" : "sell"},
            {"symbol", trade.symbol},
            {"volume", format_double_for_AST(trade.volume)},
            {"open_price", format_double_for_AST(trade.open_price)},
            {"sl", std::to_string(trade.sl)},
            {"tp", std::to_string(trade.tp)},
            {"storage", std::to_string(trade.storage)},
            {"profit", format_double_for_AST(trade.profit)},
            {"comment", trade.comment}
        });
    }

    const JSONObject table_props = table_builder.CreateTableProps();
    const Node table_node = Table({}, table_props);

    // Total report
    const Node report = div({
        h1({text("Open Trades Report")}),
        table_node
    });

    utils::CreateUI(report, response, allocator);
}