#pragma once

#include <map>
#include <string>
#include <vector>

enum class DisplayMode { Download, Upload, Both };
enum class MetricFormat { Raw, Bytes };

struct RateValue
{
    double value;
    bool megabits;
};

struct PayloadData
{
    RateValue upload;
    RateValue download;
    double cpu;
    int online;
};

struct FieldValue
{
    std::wstring raw;
    bool numeric;
    double number;
};

struct MetricDefinition
{
    std::wstring path;
    std::wstring templateText;
    MetricFormat format;
};

using FieldTable = std::map<std::wstring, FieldValue>;

bool BuildFieldTable(const std::wstring& json, FieldTable& fields) noexcept;
bool TryGetLegacyPayload(const FieldTable& fields, PayloadData& result) noexcept;
bool TryGetNumericField(const FieldTable& fields, const std::wstring& path, double& value) noexcept;
std::wstring RenderMetrics(const FieldTable& fields,
    const std::vector<MetricDefinition>& metrics, const std::wstring& separator) noexcept;
std::wstring FormatBytes(double bytes);

// V2 compatibility helpers retained for fallback mode.
bool ParsePayload(const std::wstring& json, PayloadData& result) noexcept;
std::wstring FormatRate(const RateValue& rate);
std::wstring FormatDisplay(const PayloadData& data, DisplayMode mode);
std::wstring FormatTooltip(const PayloadData& data);
