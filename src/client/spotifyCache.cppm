module;
#include <SQLiteCpp/SQLiteCpp.h>
#include <rfl/json.hpp>
#include <rfl.hpp>
export module spotify:cache;

import std;

class SpotifyCache {
public:
    SpotifyCache();
    std::optional<std::string> getFromCache(const std::string &key);


    void saveToCache(const std::string &key, const auto &body) {
        const std::string json_body = rfl::json::write(body);
        SQLite::Statement stmt(
                db,
                "INSERT OR REPLACE INTO api_cache (cache_key, response_body) VALUES (?, ?)");
        stmt.bind(1, key);
        stmt.bind(2, json_body);
        stmt.exec();
    }

private:
    SQLite::Database db;
};

SpotifyCache::SpotifyCache()
        : db("spocli.db", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE) {
    db.exec("CREATE TABLE IF NOT EXISTS api_cache ("
            "  cache_key     TEXT PRIMARY KEY,"
            "  response_body TEXT NOT NULL"
            ")");
}

std::optional<std::string> SpotifyCache::getFromCache(const std::string &key) {
    SQLite::Statement query(db, "SELECT response_body FROM api_cache "
                                "WHERE cache_key = ?");
    query.bind(1, key);
    if (query.executeStep()) {
        return query.getColumn(0).getString();
    }
    return std::nullopt;
}
