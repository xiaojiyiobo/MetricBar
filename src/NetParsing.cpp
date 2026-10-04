#include "NetParsing.h"

#include <cmath>
#include <cwchar>
#include <cwctype>

namespace
{
    const size_t kMaxDepth = 32;
    const size_t kMaxFields = 1024;

    bool HexDigit(wchar_t c, unsigned& value)
    {
        if (c >= L'0' && c <= L'9') value = static_cast<unsigned>(c - L'0');
        else if (c >= L'a' && c <= L'f') value = static_cast<unsigned>(c - L'a' + 10);
        else if (c >= L'A' && c <= L'F') value = static_cast<unsigned>(c - L'A' + 10);
        else return false;
        return true;
    }

    class JsonFlattener
    {
    public:
        JsonFlattener(const std::wstring& json, FieldTable& fields)
            : json_(json), fields_(fields), position_(0) {}

        bool Parse()
        {
            fields_.clear();
            SkipWhitespace();
            if (!ParseValue(L"", 0)) return false;
            SkipWhitespace();
            return position_ == json_.size();
        }

    private:
        void SkipWhitespace()
        {
            while (position_ < json_.size() && iswspace(json_[position_])) ++position_;
        }

        bool ParseValue(const std::wstring& path, size_t depth)
        {
            if (depth > kMaxDepth || fields_.size() > kMaxFields) return false;
            SkipWhitespace();
            if (position_ >= json_.size()) return false;
            const wchar_t c = json_[position_];
            if (c == L'{') return ParseObject(path, depth + 1);
            if (c == L'[') return ParseArray(path, depth + 1);
            if (c == L'\"')
            {
                std::wstring text;
                if (!ParseString(text)) return false;
                return AddField(path, text, false, 0.0);
            }
            if (c == L'-' || (c >= L'0' && c <= L'9')) return ParseNumber(path);
            if (MatchLiteral(L"true")) return AddField(path, L"true", false, 0.0);
            if (MatchLiteral(L"false")) return AddField(path, L"false", false, 0.0);
            if (MatchLiteral(L"null")) return AddField(path, L"null", false, 0.0);
            return false;
        }

        bool ParseObject(const std::wstring& path, size_t depth)
        {
            ++position_;
            SkipWhitespace();
            if (position_ < json_.size() && json_[position_] == L'}')
            {
                ++position_;
                return true;
            }
            for (;;)
            {
                SkipWhitespace();
                std::wstring key;
                if (!ParseString(key)) return false;
                SkipWhitespace();
                if (position_ >= json_.size() || json_[position_] != L':') return false;
                ++position_;
                const std::wstring childPath = path.empty() ? key : path + L"." + key;
                if (!ParseValue(childPath, depth)) return false;
                SkipWhitespace();
                if (position_ >= json_.size()) return false;
                if (json_[position_] == L'}')
                {
                    ++position_;
                    return true;
                }
                if (json_[position_] != L',') return false;
                ++position_;
            }
        }

        bool ParseArray(const std::wstring& path, size_t depth)
        {
            ++position_;
            SkipWhitespace();
            if (position_ < json_.size() && json_[position_] == L']')
            {
                ++position_;
                return true;
            }
            size_t index = 0;
            for (;;)
            {
                wchar_t number[32] = {};
                std::swprintf(number, 32, L"%llu", static_cast<unsigned long long>(index));
                const std::wstring childPath = path.empty() ? number : path + L"." + number;
                if (!ParseValue(childPath, depth)) return false;
                ++index;
                SkipWhitespace();
                if (position_ >= json_.size()) return false;
                if (json_[position_] == L']')
                {
                    ++position_;
                    return true;
                }
                if (json_[position_] != L',') return false;
                ++position_;
            }
        }

        bool ParseString(std::wstring& output)
        {
            if (position_ >= json_.size() || json_[position_] != L'\"') return false;
            ++position_;
            output.clear();
            while (position_ < json_.size())
            {
                wchar_t c = json_[position_++];
                if (c == L'\"') return true;
                if (c < 0x20) return false;
                if (c != L'\\')
                {
                    output.push_back(c);
                    continue;
                }
                if (position_ >= json_.size()) return false;
                c = json_[position_++];
                switch (c)
                {
                case L'\"': output.push_back(L'\"'); break;
                case L'\\': output.push_back(L'\\'); break;
                case L'/': output.push_back(L'/'); break;
                case L'b': output.push_back(L'\b'); break;
                case L'f': output.push_back(L'\f'); break;
                case L'n': output.push_back(L'\n'); break;
                case L'r': output.push_back(L'\r'); break;
                case L't': output.push_back(L'\t'); break;
                case L'u':
                {
                    unsigned code = 0;
                    if (!ParseHex4(code)) return false;
                    output.push_back(static_cast<wchar_t>(code));
                    break;
                }
                default: return false;
                }
            }
            return false;
        }

        bool ParseHex4(unsigned& code)
        {
            if (position_ + 4 > json_.size()) return false;
            code = 0;
            for (int i = 0; i < 4; ++i)
            {
                unsigned digit = 0;
                if (!HexDigit(json_[position_++], digit)) return false;
                code = (code << 4) | digit;
            }
            return true;
        }

        bool ParseNumber(const std::wstring& path)
        {
            const size_t start = position_;
            if (json_[position_] == L'-') ++position_;
            if (position_ >= json_.size()) return false;
            if (json_[position_] == L'0') ++position_;
            else
            {
                if (json_[position_] < L'1' || json_[position_] > L'9') return false;
                while (position_ < json_.size() && json_[position_] >= L'0' && json_[position_] <= L'9')
                    ++position_;
            }
            if (position_ < json_.size() && json_[position_] == L'.')
            {
                ++position_;
                const size_t fractionStart = position_;
                while (position_ < json_.size() && json_[position_] >= L'0' && json_[position_] <= L'9')
                    ++position_;
                if (position_ == fractionStart) return false;
            }
            if (position_ < json_.size() && (json_[position_] == L'e' || json_[position_] == L'E'))
            {
                ++position_;
                if (position_ < json_.size() && (json_[position_] == L'+' || json_[position_] == L'-'))
                    ++position_;
                const size_t exponentStart = position_;
                while (position_ < json_.size() && json_[position_] >= L'0' && json_[position_] <= L'9')
                    ++position_;
                if (position_ == exponentStart) return false;
            }
            const std::wstring raw = json_.substr(start, position_ - start);
            wchar_t* end = nullptr;
            const double number = std::wcstod(raw.c_str(), &end);
            if (!end || *end != L'\0' || !std::isfinite(number)) return false;
            return AddField(path, raw, true, number);
        }

        bool MatchLiteral(const wchar_t* literal)
        {
            const size_t length = std::wcslen(literal);
            if (json_.compare(position_, length, literal) != 0) return false;
            position_ += length;
            return true;
        }

        bool AddField(const std::wstring& path, const std::wstring& raw, bool numeric, double number)
        {
            if (fields_.size() >= kMaxFields) return false;
            fields_[path.empty() ? L"$" : path] = FieldValue{raw, numeric, number};
            return true;
        }

        const std::wstring& json_;
        FieldTable& fields_;
        size_t position_;
    };

    bool ParseNetwork(const std::wstring& text, RateValue& upload, RateValue& download)
    {
        const size_t up = text.find(L'\x2191');
        const size_t down = text.find(L'\x2193');
        if (up == std::wstring::npos || down == std::wstring::npos) return false;
        auto parseAt = [&text](size_t position, RateValue& rate) -> bool
        {
            const wchar_t* begin = text.c_str() + position + 1;
            while (*begin && iswspace(*begin)) ++begin;
            wchar_t* end = nullptr;
            rate.value = std::wcstod(begin, &end);
            if (end == begin || !std::isfinite(rate.value) || rate.value < 0.0) return false;
            while (*end && iswspace(*end)) ++end;
            if (_wcsnicmp(end, L"Mbps", 4) == 0) rate.megabits = true;
            else if (_wcsnicmp(end, L"Kbps", 4) == 0) rate.megabits = false;
            else return false;
            return true;
        };
        return parseAt(up, upload) && parseAt(down, download);
    }

    std::wstring ReplaceValue(const std::wstring& text, const std::wstring& value)
    {
        std::wstring output = text;
        const std::wstring marker = L"{value}";
        size_t position = 0;
        bool replaced = false;
        while ((position = output.find(marker, position)) != std::wstring::npos)
        {
            output.replace(position, marker.size(), value);
            position += value.size();
            replaced = true;
        }
        return replaced ? output : value;
    }

    void TrimTrailingZero(std::wstring& text)
    {
        const size_t suffix = text.size() - 1;
        if (suffix >= 2 && text[suffix - 2] == L'.' && text[suffix - 1] == L'0')
            text.erase(suffix - 2, 2);
    }
}

bool BuildFieldTable(const std::wstring& json, FieldTable& fields) noexcept
{
    try
    {
        JsonFlattener parser(json, fields);
        if (!parser.Parse())
        {
            fields.clear();
            return false;
        }
        const auto network = fields.find(L"vps_net");
        if (network != fields.end())
        {
            RateValue upload = {};
            RateValue download = {};
            if (ParseNetwork(network->second.raw, upload, download))
            {
                fields[L"net.up"] = FieldValue{FormatRate(upload), false, 0.0};
                fields[L"net.down"] = FieldValue{FormatRate(download), false, 0.0};
            }
        }
        return true;
    }
    catch (...)
    {
        fields.clear();
        return false;
    }
}

bool TryGetNumericField(const FieldTable& fields, const std::wstring& path, double& value) noexcept
{
    try
    {
        const auto item = fields.find(path);
        if (item == fields.end() || !item->second.numeric) return false;
        value = item->second.number;
        return std::isfinite(value);
    }
    catch (...) { return false; }
}

bool TryGetLegacyPayload(const FieldTable& fields, PayloadData& result) noexcept
{
    try
    {
        const auto network = fields.find(L"vps_net");
        double cpu = 0.0;
        double online = 0.0;
        if (network == fields.end() || !ParseNetwork(network->second.raw, result.upload, result.download) ||
            !TryGetNumericField(fields, L"vps_cpu", cpu) || !TryGetNumericField(fields, L"online", online))
            return false;
        result.cpu = cpu;
        result.online = static_cast<int>(online);
        return true;
    }
    catch (...) { return false; }
}

bool ParsePayload(const std::wstring& json, PayloadData& result) noexcept
{
    FieldTable fields;
    return BuildFieldTable(json, fields) && TryGetLegacyPayload(fields, result);
}

std::wstring FormatBytes(double bytes)
{
    if (!std::isfinite(bytes)) return L"--";
    const wchar_t* units[] = {L"B", L"K", L"M", L"G"};
    size_t unit = 0;
    double value = bytes;
    while (std::fabs(value) >= 1024.0 && unit < 3)
    {
        value /= 1024.0;
        ++unit;
    }
    wchar_t buffer[64] = {};
    if (unit == 0) std::swprintf(buffer, 64, L"%.0fB", value);
    else std::swprintf(buffer, 64, L"%.1f%ls", value, units[unit]);
    return buffer;
}

std::wstring RenderMetrics(const FieldTable& fields,
    const std::vector<MetricDefinition>& metrics, const std::wstring& separator) noexcept
{
    try
    {
        std::wstring output;
        for (size_t i = 0; i < metrics.size(); ++i)
        {
            if (i) output += separator;
            const MetricDefinition& metric = metrics[i];
            const auto field = fields.find(metric.path);
            std::wstring value = L"--";
            if (field != fields.end())
            {
                if (metric.format == MetricFormat::Raw) value = field->second.raw;
                else if (field->second.numeric) value = FormatBytes(field->second.number);
            }
            output += ReplaceValue(metric.templateText.empty() ? L"{value}" : metric.templateText, value);
        }
        return output.empty() ? L"--" : output;
    }
    catch (...) { return L"--"; }
}

std::wstring FormatRate(const RateValue& rate)
{
    wchar_t buffer[32] = {};
    if (rate.megabits)
    {
        if (rate.value >= 100.0)
            std::swprintf(buffer, 32, L"%.0fM", std::floor(rate.value + 0.5));
        else
        {
            std::swprintf(buffer, 32, L"%.1fM", rate.value);
            std::wstring text(buffer);
            TrimTrailingZero(text);
            return text;
        }
    }
    else
        std::swprintf(buffer, 32, L"%.0fK", std::floor(rate.value + 0.5));
    return buffer;
}

std::wstring FormatDisplay(const PayloadData& data, DisplayMode mode)
{
    if (mode == DisplayMode::Download) return std::wstring(L"\x2193") + FormatRate(data.download);
    if (mode == DisplayMode::Upload) return std::wstring(L"\x2191") + FormatRate(data.upload);
    return std::wstring(L"\x2193") + FormatRate(data.download) + L"  \x2191" + FormatRate(data.upload);
}

std::wstring FormatTooltip(const PayloadData& data)
{
    wchar_t buffer[160] = {};
    std::swprintf(buffer, 160, L"\x4e0b\x884c %ls  \x4e0a\x884c %ls  CPU %.1f%%  \x5728\x7ebf %d",
        FormatRate(data.download).c_str(), FormatRate(data.upload).c_str(), data.cpu, data.online);
    return buffer;
}
