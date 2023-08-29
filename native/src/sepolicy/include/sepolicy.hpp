#pragma once

#include <stdlib.h>
#include <string>

#include <selinux.hpp>

using token_list = std::vector<const char *>;
using argument = std::pair<token_list, bool>;
using argument_list = std::vector<argument>;

const argument &all_xperm();

#define ALL       nullptr
#define ALL_XPERM all_xperm()

struct sepolicy {
    using c_str = const char *;

    // Public static factory functions
    static sepolicy *from_data(char *data, size_t len);
    static sepolicy *from_file(c_str file);
    static sepolicy *from_split();
    static sepolicy *compile_split();

    // External APIs
    bool to_file(c_str file);
    void parse_statement(c_str stmt, int len);
    void parse_statement(c_str stmt) { parse_statement(stmt, strlen(stmt)); }
    void load_rules(const std::string &rules);
    void load_rule_file(c_str file);
    void print_rules();

    // Operation on types
    bool type(c_str name, c_str attr);
    bool attribute(c_str name);
    bool permissive(c_str type);
    bool enforce(c_str type);
    bool typeattribute(c_str type, c_str attr);
    bool exists(c_str type);

    // Access vector rules
    bool allow(c_str src, c_str tgt, c_str cls, c_str perm);
    bool deny(c_str src, c_str tgt, c_str cls, c_str perm);
    bool auditallow(c_str src, c_str tgt, c_str cls, c_str perm);
    bool dontaudit(c_str src, c_str tgt, c_str cls, c_str perm);

    // Extended permissions access vector rules
    bool allowxperm(c_str src, c_str tgt, c_str cls, const argument &xperm);
    bool auditallowxperm(c_str src, c_str tgt, c_str cls, const argument &xperm);
    bool dontauditxperm(c_str src, c_str tgt, c_str cls, const argument &xperm);

    // Type rules
    bool type_transition(c_str src, c_str tgt, c_str cls, c_str def, c_str obj = nullptr);
    bool type_change(c_str src, c_str tgt, c_str cls, c_str def);
    bool type_member(c_str src, c_str tgt, c_str cls, c_str def);

    // File system labeling
    bool genfscon(c_str fs_name, c_str path, c_str ctx);

    // Magisk
    void magisk_rules();

    // Deprecate
    bool create(c_str name) { return type(name, "domain"); }

protected:
    // Prevent anyone from accidentally creating an instance
    sepolicy() = default;
};
