#include "PluginInterface.h"

extern "C" int GetReportApiVersion() {
    return ReportServerInterface::GetApiVersion();
}

extern "C" void AboutReport(rapidjson::Value&                   request,
                            rapidjson::Value&                   response,
                            rapidjson::Document::AllocatorType& allocator,
                            ReportServerInterface*              server) {
    response.AddMember("version", 1, allocator);
    response.AddMember("name", Value().SetString("Open Trades report", allocator), allocator);
    response.AddMember("description",
                       Value().SetString("Summary data on open trades executed by a selected group "
                                         "of traders over a specified day. "
                                         "Includes date, symbol, price, profit, volume, s / l, t / "
                                         "p, commission, swap and account information.",
                                         allocator),
                       allocator);
    response.AddMember("type", static_cast<int>(ReportType::DailyGroup), allocator);
    response.AddMember("key", Value().SetString("OPEN_TRADES_REPORT", allocator), allocator);
}

extern "C" void DestroyReport() {}

extern "C" void CreateReport(rapidjson::Value&                   request,
                             rapidjson::Value&                   response,
                             rapidjson::Document::AllocatorType& allocator,
                             ReportServerInterface*              server) {
    // Validation
    constexpr ReportType   report_type = ReportType::DailyGroup;
    const ValidationResult validation_result =
        RequestValidator::ValidateRequest(report_type, request, server);

    if (!validation_result.allowed) {
        std::cerr << "[OpenTradesReportInterface]: " << validation_result.code
                  << ", message: " << validation_result.message << std::endl;

        const Node report =
            div({h1({text("Access Denied")},
                    props({{"style", JSONValue(JSONObject{{"color", JSONValue("#dc2626")}})}})),
                 h2({text("Code: " + std::to_string(validation_result.code))}),
                 h2({text(validation_result.message)},
                    props({{"style", JSONValue(JSONObject{{"color", JSONValue("gray")}})}}))});

        utils::CreateUI(report, response, allocator);

        return;
    }

    std::cout << "[OpenTradesReportInterface]: " << validation_result.code
              << ", message: " << validation_result.message << std::endl;

    // Execution
    std::string requested_group_mask = request["group"].GetString();
    int         from                 = request["from"].GetInt();
    int         to                   = request["to"].GetInt();
    std::string allowed_group_mask   = request["__access"]["groups"].GetString();
    std::string group_mask =
        requested_group_mask == "*" ? allowed_group_mask : requested_group_mask;

    double                                  total_volume = 0;
    std::unordered_map<std::string, double> total_map;
    std::vector<ReportTradeRecord>          trades_vector;
    std::vector<ReportGroupRecord>          groups_vector;

    try {
        server->GetOpenTradesByGroup(group_mask, from, to, &trades_vector);
        server->GetAllGroups(&groups_vector);
    } catch (const std::exception& e) {
        std::cerr << "[OpenTradesReportInterface]: " << e.what() << std::endl;
    }

    // Main table
    TableBuilder table_builder("OpenTradesReportTable");

    // Table props
    table_builder.SetIdColumn("order");
    table_builder.SetOrderBy("order", "DESC");
    table_builder.EnableAutoSave(false);
    table_builder.EnableRefreshButton(false);
    table_builder.EnableBookmarksButton(false);
    table_builder.EnableExportButton(true);
    table_builder.EnableTotal(true);
    table_builder.SetTotalDataTitle("TOTAL");

    // Filters
    FilterConfig search_filter;
    search_filter.type = FilterType::Search;

    FilterConfig date_time_filter;
    date_time_filter.type = FilterType::DateTime;

    // Columns
    table_builder.AddColumn({"order", "ORDER", 1, search_filter});
    table_builder.AddColumn({"login", "LOGIN", 2, search_filter});
    table_builder.AddColumn({"name", "NAME", 3, search_filter});
    table_builder.AddColumn({"open_time", "OPEN_TIME", 4, date_time_filter});
    table_builder.AddColumn({"type", "TYPE", 5});
    table_builder.AddColumn({"symbol", "SYMBOL", 6, search_filter});
    table_builder.AddColumn({"volume", "VOLUME", 7, search_filter});
    table_builder.AddColumn({"open_price", "OPEN_PRICE", 8, search_filter});
    table_builder.AddColumn({"sl", "S / L", 9, search_filter});
    table_builder.AddColumn({"tp", "T / P", 10, search_filter});
    table_builder.AddColumn({"storage", "SWAP", 11, search_filter});
    table_builder.AddColumn({"profit", "AMOUNT", 12, search_filter});
    table_builder.AddColumn({"comment", "COMMENT", 13, search_filter});
    table_builder.AddColumn({"currency", "CURRENCY", 14, search_filter});
    table_builder.AddColumn({"group", "GROUP", 15, search_filter});

    for (const auto& trade : trades_vector) {
        ReportAccountRecord account;

        try {
            server->GetAccountByLogin(trade.login, &account);
        } catch (const std::exception& e) {
            std::cerr << "[OpenTradesReportInterface]: " << e.what() << std::endl;
        }
        const std::string currency   = utils::GetGroupCurrencyByName(groups_vector, account.group);
        double            multiplier = 1;

        total_map[currency] += trade.volume;

        table_builder.AddRow({utils::TruncateDouble(trade.order, 0),
                              utils::TruncateDouble(trade.login, 0),
                              account.name,
                              utils::FormatTimestampToString(trade.open_time),
                              utils::ConvertCmdToString(static_cast<int>(trade.cmd)),
                              trade.symbol,
                              utils::TruncateDouble(trade.volume / 100.0, 2),
                              utils::TruncateDouble(trade.open_price * multiplier, 2),
                              utils::TruncateDouble(trade.sl * multiplier, 2),
                              utils::TruncateDouble(trade.tp * multiplier, 2),
                              utils::TruncateDouble(trade.storage * multiplier, 2),
                              utils::TruncateDouble(trade.profit * multiplier, 2),
                              trade.comment,
                              currency,
                              account.group});
    }

    // Total row
    JSONArray totals_array;
    for (const auto& [currency, total_volume] : total_map) {
        totals_array.emplace_back(JSONObject{
            {"volume", utils::TruncateDouble(total_volume / 100.0, 2)}, {"currency", currency}});
    }

    table_builder.SetTotalData(totals_array);

    const JSONObject table_props = table_builder.CreateTableProps();
    const Node       table_node  = Table({}, table_props);

    // Total report
    const Node report = Column({h1({text("Open Trades Report")}), table_node});

    utils::CreateUI(report, response, allocator);
}