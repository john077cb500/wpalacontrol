#ifndef JOTUL_COMMAND_POLICY_H
#define JOTUL_COMMAND_POLICY_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>

// Transport-independent policy, exercised by host tests. Call only after TLS
// readiness and strict JSON type/size validation. Session token is not a secret.
class JotulCommandPolicy
{
    uint32_t lastSequence = 0;
    uint32_t lastMillis = 0;
    bool seen = false;
public:
    void reset() { lastSequence = 0; lastMillis = 0; seen = false; }
    bool accept(const char *session, size_t sessionLength,
                const char *expected, size_t expectedLength,
                uint32_t sequence, uint32_t timestamp, int64_t now,
                uint32_t uptime, const char *command, size_t commandLength)
    {
        if (!session || !expected || !command || expectedLength != 32 ||
            sessionLength != expectedLength || memcmp(session, expected, expectedLength) ||
            sequence <= lastSequence || now - timestamp < 0 || now - timestamp > 30 ||
            (seen && uint32_t(uptime - lastMillis) < 2000))
            return false;
        const char *allowed[] = {"GET STAT", "GET TMPS", "GET FAND", "GET CNTR",
                                 "GET TIME", "GET SETP", "GET POWR", "GET DPRS"};
        for (const char *entry : allowed)
            if (commandLength == strlen(entry) && !memcmp(command, entry, commandLength))
            {
                lastSequence = sequence;
                lastMillis = uptime;
                seen = true;
                return true;
            }
        return false;
    }
};
#endif
