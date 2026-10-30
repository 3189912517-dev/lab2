#include "defuser.h"

#include <inttypes.h>
#include <stdio.h>

#define MAX_FIELDS 8

/*
 * 把 values 中的 count 个整数写成一行“空格分隔的无符号十进制整数”，存入 out。
 * 成功返回 0；count 为 0（还没有实现）、snprintf 出错或结果被截断时返回非零。
 */
static int write_answer(char *out, size_t cap, const uint32_t *values, size_t count) {
    size_t used = 0;
    for (size_t i = 0; i < count; ++i) {
        int size = snprintf(out + used, cap - used, "%s%" PRIu32, i ? " " : "", values[i]);
        if (size < 0 || (size_t)size >= cap - used) return 1;
        used += (size_t)size;
    }
    return count == 0;
}

int solve_phase_1(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    uint32_t hi = (uint32_t)(challenge >> 32);
    uint32_t mid = (uint32_t)(challenge >> 16);
    uint32_t first = (hi * 5u + mid) ^ 0xb2fau;
    uint32_t second = (uint32_t)challenge * 5u ^ (hi + 0x32a8u);
    values[0] = (first + 0xf0afu) ^ second;
    return write_answer(out, cap, values, 1);
}

int solve_phase_2(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    values[0] = (uint32_t)((challenge >> 21) & 31u) * 3u;
    for (uint32_t i = 1; i < 6; ++i) {
        uint32_t nibble = (uint32_t)((challenge >> (22u + 4u * i)) & 15u);
        values[i] = (nibble + i) ^ (values[i - 1] * 5u);
    }
    return write_answer(out, cap, values, 6);
}

int solve_phase_3(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    uint32_t index = (uint32_t)challenge & 7u;
    uint32_t arg = (uint32_t)(challenge >> 3) ^ (uint32_t)(challenge >> 32);
    values[0] = index;
    switch (index) {
        case 0: values[1] = arg * 2u ^ 0x707cu; break;
        case 1: values[1] = arg * 5u + 0x2fdebu; break;
        case 2: values[1] = arg * 3u + 0x52f4u; break;
        case 3: values[1] = (arg ^ 0x9a64u) + (arg >> 1); break;
        case 4: values[1] = arg * 5u + 0x2f7bfu; break;
        case 5: values[1] = arg * 4u ^ 0xf7aau; break;
        case 6: values[1] = arg * 5u + 0x477bu; break;
        default: values[1] = (arg ^ 0x6062u) + (arg >> 2); break;
    }
    return write_answer(out, cap, values, 2);
}

static uint32_t p4_walk(uint32_t target, uint32_t lo, uint32_t hi, uint32_t salt) {
    uint32_t mid = lo + ((hi - lo) >> 1);
    if (target == mid) return (mid + 0x59u) ^ salt;
    uint32_t child = target < mid
        ? p4_walk(target, lo, mid - 1u, salt)
        : p4_walk(target, mid + 1u, hi, salt);
    return ((child * 3u) ^ (mid + salt)) ^ (target < mid ? 7u : 0x15u);
}

int solve_phase_4(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    uint32_t target = (uint32_t)challenge & 127u;
    uint32_t salt = (uint32_t)(challenge >> 7) ^ (uint32_t)(challenge >> 32);
    values[0] = p4_walk(target, 0, 127, salt);
    return write_answer(out, cap, values, 1);
}

struct p5_node { uint8_t next; uint32_t value; uint32_t add; };
static const struct p5_node p5_nodes[12] = {
    {5, 0x918, 0x47b1}, {2, 0x56, 0x102f},
    {0, 0x565, 0x641d}, {3, 0x405, 0x988c},
    {9, 0x318, 0x950b}, {7, 0x254, 0x1f9c},
    {3, 0x623, 0x6684}, {6, 0x778, 0xcef7},
    {11, 0xb2d, 0xa6b9}, {1, 0xa33, 0x5f6b},
    {8, 0x86a, 0x068f}, {4, 0x103, 0x99db}
};
static const uint8_t p5_heads[8] = {10, 8, 11, 4, 9, 1, 2, 0};

int solve_phase_5(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    uint8_t node = p5_heads[challenge & 7u];
    uint32_t steps = (uint32_t)(challenge >> 3) & 3u;
    uint32_t mixed = ((uint32_t)(challenge >> 6) ^ (uint32_t)(challenge >> 32)) ^ 0x7632u;
    mixed = (mixed * 3u + p5_nodes[node].add) ^ p5_nodes[node].value;
    for (uint32_t i = 0; i < steps; ++i) {
        node = p5_nodes[node].next;
        mixed = (mixed * 3u + p5_nodes[node].add) ^ p5_nodes[node].value;
    }
    values[0] = mixed;
    values[1] = p5_nodes[node].value;
    return write_answer(out, cap, values, 2);
}

static uint32_t p6_op(unsigned id, uint32_t arg) {
    switch (id) {
        case 0: return (arg ^ 0x831eu) + arg * 8u;
        case 1: return ((arg >> 1) ^ arg) + 0xef36u;
        case 2: return (arg >> 3) | (arg << 29);
        case 3: return (arg + 0x6cf2u) ^ (arg >> 3);
        case 4: return (arg * 8u + 0x84c4u) ^ (arg * 5u);
        case 5: return ((arg >> 29) | (arg << 3)) ^ 0x7b6eu;
        default: return arg;
    }
}

int solve_phase_6(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    static const uint8_t ops[8] = {0, 1, 2, 3, 4, 4, 5, 4};
    uint32_t arg = (uint32_t)(challenge >> 6) ^ (uint32_t)(challenge >> 32);
    uint32_t first = p6_op(ops[challenge & 7u], arg) ^ 0xebf5u;
    values[0] = p6_op(ops[(challenge >> 3) & 7u], first) + 0x5d80u;
    return write_answer(out, cap, values, 1);
}

int solve_secret(uint64_t challenge, uint64_t context, char *out, size_t cap) {
    uint32_t select = (uint32_t)context & 3u;
    uint32_t ch = (uint32_t)(challenge >> 32) ^ (uint32_t)challenge;
    uint32_t ctx = (uint32_t)(context >> 32) ^ (uint32_t)context;
    uint32_t second = ((ch ^ ctx) * 5u + 0xbb96u) ^ 0x5407u;
    second ^= select * 0xbb96u ^ (uint32_t)(context >> 16);
    uint32_t third = second ^ (ctx + select + 0x5407u + (ch >> 7));
    int size = snprintf(out, cap,
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAOPEN%" PRIu32 " %" PRIu32 " %" PRIu32,
        select, second, third);
    return size < 0 || (size_t)size >= cap;
}
