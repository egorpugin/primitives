#pragma once

#include <primitives/filesystem.h>
#include <primitives/stdcompat/optional.h>

namespace primitives::db
{

/// lite db: has execute() call
/// kv db accessor in column manner: has getValue()/setValue() calls
struct LiteDatabase
{
    virtual ~LiteDatabase() = default;

    virtual void execute(const String &) = 0;

    template <typename T>
    optional<T> getValue(const String &key)
    {
        checkKey(key);
        auto v = getValueByKey(key);
        if (!v)
            return {};
        if constexpr (std::is_same_v<T, int>)
            return std::stoi(v.value());
        else if constexpr (std::is_same_v<T, double>)
            return std::stod(v.value());
        else
            return v;
    }

    template <typename T>
    void setValue(const String &key, const T &v)
    {
        checkKey(key);
        if constexpr (std::is_convertible_v<T, String>)
            setValueByKey(key, v);
        else
            setValueByKey(key, std::to_string(v));
    }

private:
    virtual optional<String> getValueByKey(const String &key) = 0;
    virtual void setValueByKey(const String &key, const String &value) = 0;
    void checkKey(const String &key);
};

void createOrUpdateSchema(LiteDatabase &database, const String &latestSchema, const Strings &diffSqls);
void createOrUpdateSchema(LiteDatabase &database, const path &latestSchemaFilename, const path &diffSqlsDir, const String &diffSqlsFileMask);

}
