//
// Created by vanilla on 2020/7/18.
//

#include "string"
#include "cpr/cpr.h"
#include "json/json.h"

#include "boost/assign/list_of.hpp"
#include "boost/unordered_map.hpp"

#include "fmt/core.h"

using namespace std;

class Weather {
public:
    string date = "undefined";
    double temperatureAvg = -1;
    double airQualityPm25Avg = -1;
    string skyConValue = "undefined";

    // @NoArgsConstructor
    Weather() = default;

    // @AllArgsConstructor
    Weather(string date, double temperatureAvg, double airQualityPm25Avg, string skyConValue) :
            date(move(date)), temperatureAvg(temperatureAvg),
            airQualityPm25Avg(airQualityPm25Avg), skyConValue(move(skyConValue)) {}
};

/**
 * get coordinate by using given location info.
 *
 * @param location, example: "北京"
 * @return empty string if no result.
 */
string getCoordinate(const string &location) {
    const string GEO_API_BASE = "https://restapi.amap.com/v3/geocode/geo";
    const string GEO_API_KEY = "4ea6a42cd57fd368acd56c1cb2fd51a1";

    auto request = cpr::Get(cpr::Url{GEO_API_BASE},
                            cpr::Parameters{{"key",     GEO_API_KEY},
                                            {"address", location}});

    Json::Value json;
    istringstream requestStringStream(request.text);
    requestStringStream >> json;

    if (json["count"].asString() == "0") return "";
    return json["geocodes"][0]["location"].asString();
}

/**
 * get recent 5 days weather, by using given location.
 *
 * @param location need to query coordinate.
 * @return a list of weather, size should be 5.
 */
vector<Weather> getWeatherList(const string &location) {
    string coordinate = getCoordinate(location);
    if (coordinate.empty()) return vector<Weather>();

    const string WEATHER_API_BASE = "https://api.caiyunapp.com/v2.5/";
    const string WEATHER_TOKEN = "Xdu5hktYFvaDlnCT";
    const string DAILY_JSON = "/daily.json";

    auto request = cpr::Get(cpr::Url{WEATHER_API_BASE + WEATHER_TOKEN + "/"
                                     + coordinate + DAILY_JSON});

    Json::Value json;
    istringstream requestStringStream(request.text);
    requestStringStream >> json;

    Json::Value dailyJson = json["result"]["daily"];
    string status = dailyJson["status"].asString();
    if (status != "ok") return vector<Weather>();

    vector<Weather> weatherList = vector<Weather>();
    for (int i = 0; i < 5; ++i) {
        double temperatureAvg = dailyJson["temperature"][i]["avg"].asDouble();
        string date = dailyJson["temperature"][i]["date"].asString().substr(0, 10);
        double airQualityPm25Avg = dailyJson["air_quality"]["pm25"][i]["avg"].asDouble();
        string skyConValue = dailyJson["skycon"][i]["value"].asString();

        Weather weather = Weather(date, temperatureAvg, airQualityPm25Avg, skyConValue);
        weatherList.push_back(weather);
    }
    return weatherList;
}

using namespace boost;
#pragma ide diagnostic ignored "cert-err58-cpp"
const unordered_map <string, string> SKY_CON_MAP =
        assign::map_list_of("CLEAR_DAY", "晴（白天）")("CLEAR_NIGHT", "晴（夜间）")
                ("PARTLY_CLOUDY_DAY", "多云（白天）")("PARTLY_CLOUDY_NIGHT", "多云（夜间）")
                ("CLOUDY", "阴")("LIGHT_HAZE", "轻度雾霾")("MODERATE_HAZE", "中度雾霾")("HEAVY_HAZE", "重度雾霾")
                ("LIGHT_RAIN", "小雨")("MODERATE_RAIN", "中雨")("HEAVY_RAIN", "大雨")("STORM_RAIN", "暴雨")
                ("FOG", "雾")("LIGHT_SNOW", "小雪")("MODERATE_SNOW", "中雪")("HEAVY_SNOW", "大雪")("STORM_SNOW", "暴雪")
                ("DUST", "浮尘")("SAND", "沙尘")("WIND", "大风");

string printableWeatherList(const vector<Weather> &weatherList) {
    using namespace fort;

    utf8_table weatherTable;

    for (int i = 0; i < 5; ++i)
        weatherTable.column(i).set_cell_text_align(text_align::center);

    weatherTable << header << "Date" << "Avg Temperature" << "Avg PM 2.5" << "Weather Condition" << endr;
    for (const Weather &weather: weatherList)
        weatherTable << weather.date << fmt::format("{:.2f}", weather.temperatureAvg) + " °C"
                     << fmt::format("{:.2f}", weather.airQualityPm25Avg) + " μg / m³"
                     << SKY_CON_MAP.find(weather.skyConValue)->second << endr;

    return weatherTable.to_string();
}