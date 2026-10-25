#include "../src/JotulCommandPolicy.h"
#include <assert.h>
#include <iostream>
int main()
{
    JotulCommandPolicy policy;
    const char *session = "0123456789abcdef0123456789abcdef";
    auto check = [&](uint32_t seq, uint32_t ts, uint32_t ms, const char *cmd,
                     const char *token = nullptr, size_t len = 8) {
        return policy.accept(token ? token : session, 32, session, 32,
                             seq, ts, 2000000000, ms, cmd, len);
    };
    assert(check(1, 2000000000, 0, "GET STAT"));
    assert(!check(1, 2000000000, 2000, "GET STAT")); // duplicate
    assert(!check(0, 2000000000, 2000, "GET STAT")); // sequence rollback
    assert(!check(2, 2000000000, 1999, "GET STAT")); // rate limit
    assert(!check(2, 1999999969, 2000, "GET STAT")); // expired
    assert(!check(2, 2000000001, 2000, "GET STAT")); // future
    assert(!check(2, 2000000000, 2000, "GET STAT", "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"));
    assert(!check(2, 2000000000, 2000, "SET POWR"));
    assert(!check(2, 2000000000, 2000, "CMD ON", nullptr, 6));
    assert(!check(2, 2000000000, 2000, "EXT ADRD"));
    assert(!check(2, 2000000000, 2000, "GET STAT\0x", nullptr, 10));
    assert(check(2, 1999999970, 2000, "GET TMPS")); // age boundary; rejects did not consume seq
    policy.reset();
    assert(check(1, 2000000000, UINT32_MAX - 999, "GET STAT"));
    assert(!check(2, 2000000000, 999, "GET STAT")); // millis rollover: 1999 ms
    assert(check(2, 2000000000, 1000, "GET POWR")); // rollover: 2000 ms
    std::cout << "Jotul command policy tests passed\n";
}
