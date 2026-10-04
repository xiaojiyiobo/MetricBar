#include "../src/NetParsing.h"

#include <iostream>

static int failures = 0;

static void Expect(const wchar_t* name, const std::wstring& expected, const std::wstring& actual)
{
    if (expected == actual) std::wcout << L"PASS " << name << L"\n";
    else
    {
        ++failures;
        std::wcerr << L"FAIL " << name << L": expected [" << expected << L"], got [" << actual << L"]\n";
    }
}

static void ExpectField(const FieldTable& fields, const wchar_t* path, const wchar_t* value)
{
    const auto item = fields.find(path);
    if (item == fields.end())
    {
        ++failures;
        std::wcerr << L"FAIL missing field " << path << L"\n";
        return;
    }
    Expect(path, value, item->second.raw);
}

int main()
{
    const std::wstring vpsJson = L"{\"vps_cpu\":16.4,\"online\":2,"
        L"\"vps_mem_free\":2097152,\"vps_net\":\"\\u21912.7Mbps \\u21932.5Mbps\","
        L"\"status\":{\"name\":\"ready\",\"online\":3},\"items\":[{\"value\":1536}]}";
    FieldTable fields;
    if (!BuildFieldTable(vpsJson, fields))
    {
        std::wcerr << L"FAIL JSON field table\n";
        return 1;
    }
    ExpectField(fields, L"vps_cpu", L"16.4");
    ExpectField(fields, L"status.name", L"ready");
    ExpectField(fields, L"status.online", L"3");
    ExpectField(fields, L"items.0.value", L"1536");
    ExpectField(fields, L"net.down", L"2.5M");
    ExpectField(fields, L"net.up", L"2.7M");

    const std::vector<MetricDefinition> defaults = {
        {L"net.down", L"\x2193{value}", MetricFormat::Raw},
        {L"net.up", L"\x2191{value}", MetricFormat::Raw},
        {L"vps_cpu", L"CPU {value}%", MetricFormat::Raw}
    };
    Expect(L"default metrics", L"\x2193" L"2.5M \x2191" L"2.7M CPU 16.4%",
        RenderMetrics(fields, defaults, L" "));

    const std::vector<MetricDefinition> missing = {
        {L"status.name", L"State={value}", MetricFormat::Raw},
        {L"does.not.exist", L"Bad={value}", MetricFormat::Raw},
        {L"vps_cpu", L"CPU={value}", MetricFormat::Raw}
    };
    Expect(L"missing field isolation", L"State=ready | Bad=-- | CPU=16.4",
        RenderMetrics(fields, missing, L" | "));

    const std::vector<MetricDefinition> bytes = {
        {L"items.0.value", L"{value}", MetricFormat::Bytes},
        {L"vps_mem_free", L"{value}", MetricFormat::Bytes}
    };
    Expect(L"bytes metrics", L"1.5K 2.0M", RenderMetrics(fields, bytes, L" "));
    Expect(L"bytes 1536", L"1.5K", FormatBytes(1536));
    Expect(L"bytes 2097152", L"2.0M", FormatBytes(2097152));

    PayloadData legacy = {};
    if (!TryGetLegacyPayload(fields, legacy))
    {
        std::wcerr << L"FAIL legacy payload\n";
        ++failures;
    }
    else
        Expect(L"legacy fallback", L"\x2193" L"2.5M  \x2191" L"2.7M",
            FormatDisplay(legacy, DisplayMode::Both));

    const std::wstring arbitrary = L"{\"name\":\"demo\",\"count\":42,\"nested\":{\"ratio\":1.25}}";
    FieldTable arbitraryFields;
    if (!BuildFieldTable(arbitrary, arbitraryFields))
    {
        std::wcerr << L"FAIL arbitrary JSON\n";
        ++failures;
    }
    else
    {
        ExpectField(arbitraryFields, L"name", L"demo");
        ExpectField(arbitraryFields, L"count", L"42");
        ExpectField(arbitraryFields, L"nested.ratio", L"1.25");
    }

    FieldTable invalid;
    if (BuildFieldTable(L"{\"broken\":}", invalid))
    {
        std::wcerr << L"FAIL invalid JSON accepted\n";
        ++failures;
    }
    else std::wcout << L"PASS invalid JSON rejected\n";
    return failures == 0 ? 0 : 1;
}
