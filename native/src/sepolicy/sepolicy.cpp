#include <base.hpp>

#include "policy.hpp"

using namespace std;

// Invert is adding rules for auditdeny; in other cases, invert is removing rules
#define strip_av(effect, invert) ((effect == AVTAB_AUDITDENY) == !invert)

// libsepol internal APIs
__BEGIN_DECLS
int policydb_index_decls(sepol_handle_t * handle, policydb_t * p);
int avtab_hash(struct avtab_key *keyp, uint32_t mask);
int type_set_expand(type_set_t * set, ebitmap_t * t, policydb_t * p, unsigned char alwaysexpand);
int context_from_string(
        sepol_handle_t * handle,
        const policydb_t * policydb,
        context_struct_t ** cptr,
        const char *con_str, size_t con_str_len);
int context_to_string(
        sepol_handle_t * handle,
        const policydb_t * policydb,
        const context_struct_t * context,
        char **result, size_t * result_len);
__END_DECLS

template <typename T>
struct auto_cast_wrapper
{
    auto_cast_wrapper(T *ptr) : ptr(ptr) {}
    template <typename U>
    operator U*() const { return static_cast<U*>(ptr); }
private:
    T *ptr;
};

template <typename T>
static auto_cast_wrapper<T> auto_cast(T *p) {
    return auto_cast_wrapper<T>(p);
}

static auto hashtab_find(hashtab_t h, const_hashtab_key_t key) {
    return auto_cast(hashtab_search(h, key));
}

template <class Node, class Func>
static void list_for_each(Node *node_ptr, const Func &fn) {
    auto cur = node_ptr;
    while (cur) {
        auto next = cur->next;
        fn(cur);
        cur = next;
    }
}

template <class Node, class Func>
static void hash_for_each(Node **node_ptr, int n_slot, const Func &fn) {
    for (int i = 0; i < n_slot; ++i) {
        list_for_each(node_ptr[i], fn);
    }
}

template <class Func>
static void hashtab_for_each(hashtab_t htab, const Func &fn) {
    hash_for_each(htab->htable, htab->size, fn);
}

template <class Func>
static void avtab_for_each(avtab_t *avtab, const Func &fn) {
    hash_for_each(avtab->htable, avtab->nslot, fn);
}

template <class Func>
static void for_each_attr(hashtab_t htab, const Func &fn) {
    hashtab_for_each(htab, [&](hashtab_ptr_t node) {
        auto type = static_cast<type_datum_t *>(node->datum);
        if (type->flavor == TYPE_ATTRIB)
            fn(type);
    });
}

static int avtab_remove_node(avtab_t *h, avtab_ptr_t node) {
    if (!h || !h->htable)
        return SEPOL_ENOMEM;
    int hvalue = avtab_hash(&node->key, h->mask);
    avtab_ptr_t prev = nullptr;
    avtab_ptr_t cur = h->htable[hvalue];
    while (cur) {
        if (cur == node)
            break;
        prev = cur;
        cur = cur->next;
    }
    if (cur == nullptr)
        return SEPOL_ENOENT;

    // Detach from link list
    if (prev)
        prev->next = node->next;
    else
        h->htable[hvalue] = node->next;
    h->nel--;

    // Free memory
    free(node->datum.xperms);
    free(node);
    return 0;
}

static bool is_redundant(avtab_ptr_t node) {
    switch (node->key.specified) {
    case AVTAB_AUDITDENY:
        return node->datum.data == ~0U;
    case AVTAB_XPERMS:
        return node->datum.xperms == nullptr;
    default:
        return node->datum.data == 0U;
    }
}

avtab_ptr_t sepol_impl::find_avtab_node(avtab_key_t *key, avtab_extended_perms_t *xperms) {
    avtab_ptr_t node;

    // AVTAB_XPERMS entries are not necessarily unique
    if (key->specified & AVTAB_XPERMS) {
        if (xperms == nullptr)
            return nullptr;
        node = avtab_search_node(&db->te_avtab, key);
        while (node) {
            if ((node->datum.xperms->specified == xperms->specified) &&
                (node->datum.xperms->driver == xperms->driver)) {
                node = nullptr;
                break;
            }
            node = avtab_search_node_next(node, key->specified);
        }
    } else {
        node = avtab_search_node(&db->te_avtab, key);
    }

    return node;
}

avtab_ptr_t sepol_impl::insert_avtab_node(avtab_key_t *key) {
    avtab_datum_t avdatum{};
    // AUDITDENY, aka DONTAUDIT, are &= assigned, versus |= for others.
    // Initialize the data accordingly.
    avdatum.data = key->specified == AVTAB_AUDITDENY ? ~0U : 0U;
    return avtab_insert_nonunique(&db->te_avtab, key, &avdatum);
}

avtab_ptr_t sepol_impl::get_avtab_node(avtab_key_t *key, avtab_extended_perms_t *xperms) {
    avtab_ptr_t node = find_avtab_node(key, xperms);
    if (!node) {
        node = insert_avtab_node(key);
    }
    return node;
}

void sepol_impl::add_rule(type_datum_t *src, type_datum_t *tgt, class_datum_t *cls, perm_datum_t *perm, int effect, bool invert) {
    if (src == nullptr) {
        if (strip_av(effect, invert)) {
            // Stripping av, have to go through all types for correct results
            hashtab_for_each(db->p_types.table, [&](hashtab_ptr_t node) {
                add_rule(auto_cast(node->datum), tgt, cls, perm, effect, invert);
            });
        } else {
            // If we are not stripping av, go through all attributes instead of types for optimization
            for_each_attr(db->p_types.table, [&](type_datum_t *type) {
                add_rule(type, tgt, cls, perm, effect, invert);
            });
        }
    } else if (tgt == nullptr) {
        if (strip_av(effect, invert)) {
            hashtab_for_each(db->p_types.table, [&](hashtab_ptr_t node) {
                add_rule(src, auto_cast(node->datum), cls, perm, effect, invert);
            });
        } else {
            for_each_attr(db->p_types.table, [&](type_datum_t *type) {
                add_rule(src, type, cls, perm, effect, invert);
            });
        }
    } else if (cls == nullptr) {
        hashtab_for_each(db->p_classes.table, [&](hashtab_ptr_t node) {
            add_rule(src, tgt, auto_cast(node->datum), perm, effect, invert);
        });
    } else {
        avtab_key_t key;
        key.source_type = src->s.value;
        key.target_type = tgt->s.value;
        key.target_class = cls->s.value;
        key.specified = effect;

        avtab_ptr_t node = get_avtab_node(&key, nullptr);
        if (invert) {
            if (perm)
                node->datum.data &= ~(1U << (perm->s.value - 1));
            else
                node->datum.data = 0U;
        } else {
            if (perm)
                node->datum.data |= 1U << (perm->s.value - 1);
            else
                node->datum.data = ~0U;
        }

        if (is_redundant(node))
            avtab_remove_node(&db->te_avtab, node);
    }
}

bool sepol_impl::add_rule(const char *s, const char *t, const char *c, const char *p, int effect, bool invert) {
    type_datum_t *src = nullptr, *tgt = nullptr;
    class_datum_t *cls = nullptr;
    perm_datum_t *perm = nullptr;

    if (s) {
        src = hashtab_find(db->p_types.table, s);
        if (src == nullptr) {
            LOGW("source type %s does not exist\n", s);
            return false;
        }
    }

    if (t) {
        tgt = hashtab_find(db->p_types.table, t);
        if (tgt == nullptr) {
            LOGW("target type %s does not exist\n", t);
            return false;
        }
    }

    if (c) {
        cls = hashtab_find(db->p_classes.table, c);
        if (cls == nullptr) {
            LOGW("class %s does not exist\n", c);
            return false;
        }
    }

    if (p) {
        if (c == nullptr) {
            LOGW("No class is specified, cannot add perm [%s] \n", p);
            return false;
        }

        perm = hashtab_find(cls->permissions.table, p);
        if (perm == nullptr && cls->comdatum != nullptr) {
            perm = hashtab_find(cls->comdatum->permissions.table, p);
        }
        if (perm == nullptr) {
            LOGW("perm %s does not exist in class %s\n", p, c);
            return false;
        }
    }
    add_rule(src, tgt, cls, perm, effect, invert);
    return true;
}

#define ioctl_driver(x) (x>>8 & 0xFF)
#define ioctl_func(x) (x & 0xFF)

void sepol_impl::add_xperm_rule(type_datum_t *src, type_datum_t *tgt, class_datum_t *cls, const argument &xperm, int effect) {
    if (src == nullptr) {
        for_each_attr(db->p_types.table, [&](type_datum_t *type) {
            add_xperm_rule(type, tgt, cls, xperm, effect);
        });
    } else if (tgt == nullptr) {
        for_each_attr(db->p_types.table, [&](type_datum_t *type) {
            add_xperm_rule(src, type, cls, xperm, effect);
        });
    } else if (cls == nullptr) {
        hashtab_for_each(db->p_classes.table, [&](hashtab_ptr_t node) {
            add_xperm_rule(src, tgt, auto_cast(node->datum), xperm, effect);
        });
    } else {
        avtab_key_t key;
        key.source_type = src->s.value;
        key.target_type = tgt->s.value;
        key.target_class = cls->s.value;
        key.specified = effect;

        // Each key may contain 1 driver node and 256 function nodes
        avtab_ptr_t node_list[257] = { nullptr };
#define driver_node (node_list[256])

        // Find all rules with key
        for (avtab_ptr_t node = avtab_search_node(&db->te_avtab, &key); node;) {
            if (node->datum.xperms->specified == AVTAB_XPERMS_IOCTLDRIVER) {
                driver_node = node;
            } else if (node->datum.xperms->specified == AVTAB_XPERMS_IOCTLFUNCTION) {
                node_list[node->datum.xperms->driver] = node;
            }
            node = avtab_search_node_next(node, key.specified);
        }

        bool reset = xperm.second;
        vector<pair<uint16_t, uint16_t>> ranges;

        for (const char *tok : xperm.first) {
            uint16_t low = 0;
            uint16_t high = 0;
            if (tok == nullptr) {
                low = 0x0000;
                high = 0xFF00;
                reset = true;
            } else if (strchr(tok, '-')) {
                if (sscanf(tok, "%hx-%hx", &low, &high) != 2) {
                    // Invalid token, skip
                    continue;
                }
            } else {
                if (sscanf(tok, "%hx", &low) != 1) {
                    // Invalid token, skip
                    continue;
                }
                high = low;
            }
            if (high == 0) {
                reset = true;
            } else {
                ranges.emplace_back(make_pair(low, high));
            }
        }

        if (reset) {
            for (int i = 0; i <= 0xFF; ++i) {
                if (node_list[i]) {
                    avtab_remove_node(&db->te_avtab, node_list[i]);
                    node_list[i] = nullptr;
                }
            }
            if (driver_node) {
                memset(driver_node->datum.xperms->perms, 0, sizeof(avtab_extended_perms_t::perms));
            }
        }

        auto new_driver_node = [&]() -> avtab_ptr_t {
            auto node = insert_avtab_node(&key);
            node->datum.xperms = auto_cast(calloc(1, sizeof(avtab_extended_perms_t)));
            node->datum.xperms->specified = AVTAB_XPERMS_IOCTLDRIVER;
            node->datum.xperms->driver = 0;
            return node;
        };

        auto new_func_node = [&](uint8_t driver) -> avtab_ptr_t {
            auto node = insert_avtab_node(&key);
            node->datum.xperms = auto_cast(calloc(1, sizeof(avtab_extended_perms_t)));
            node->datum.xperms->specified = AVTAB_XPERMS_IOCTLFUNCTION;
            node->datum.xperms->driver = driver;
            return node;
        };

        if (!xperm.second) {
            for (auto [low, high] : ranges) {
                if (ioctl_driver(low) != ioctl_driver(high)) {
                    if (driver_node == nullptr) {
                        driver_node = new_driver_node();
                    }
                    for (int i = ioctl_driver(low); i <= ioctl_driver(high); ++i) {
                        xperm_set(i, driver_node->datum.xperms->perms);
                    }
                } else {
                    uint8_t driver = ioctl_driver(low);
                    auto node = node_list[driver];
                    if (node == nullptr) {
                        node = new_func_node(driver);
                        node_list[driver] = node;
                    }
                    for (int i = ioctl_func(low); i <= ioctl_func(high); ++i) {
                        xperm_set(i, node->datum.xperms->perms);
                    }
                }
            }
        } else {
            if (driver_node == nullptr) {
                driver_node = new_driver_node();
            }
            // Fill the driver perms
            memset(driver_node->datum.xperms->perms, ~0, sizeof(avtab_extended_perms_t::perms));

            for (auto [low, high] : ranges) {
                if (ioctl_driver(low) != ioctl_driver(high)) {
                    for (int i = ioctl_driver(low); i <= ioctl_driver(high); ++i) {
                        xperm_clear(i, driver_node->datum.xperms->perms);
                    }
                } else {
                    uint8_t driver = ioctl_driver(low);
                    auto node = node_list[driver];
                    if (node == nullptr) {
                        node = new_func_node(driver);
                        // Fill the func perms
                        memset(node->datum.xperms->perms, ~0, sizeof(avtab_extended_perms_t::perms));
                        node_list[driver] = node;
                    }
                    xperm_clear(driver, driver_node->datum.xperms->perms);
                    for (int i = ioctl_func(low); i <= ioctl_func(high); ++i) {
                        xperm_clear(i, node->datum.xperms->perms);
                    }
                }
            }
        }
    }
}

bool sepol_impl::add_xperm_rule(const char *s, const char *t, const char *c, const argument &xperm, int effect) {
    type_datum_t *src = nullptr, *tgt = nullptr;
    class_datum_t *cls = nullptr;

    if (s) {
        src = hashtab_find(db->p_types.table, s);
        if (src == nullptr) {
            LOGW("source type %s does not exist\n", s);
            return false;
        }
    }

    if (t) {
        tgt = hashtab_find(db->p_types.table, t);
        if (tgt == nullptr) {
            LOGW("target type %s does not exist\n", t);
            return false;
        }
    }

    if (c) {
        cls = hashtab_find(db->p_classes.table, c);
        if (cls == nullptr) {
            LOGW("class %s does not exist\n", c);
            return false;
        }
    }

    add_xperm_rule(src, tgt, cls, xperm, effect);
    return true;
}

bool sepol_impl::add_type_rule(const char *s, const char *t, const char *c, const char *d, int effect) {
    type_datum_t *src, *tgt, *def;
    class_datum_t *cls;

    src = hashtab_find(db->p_types.table, s);
    if (src == nullptr) {
        LOGW("source type %s does not exist\n", s);
        return false;
    }
    tgt = hashtab_find(db->p_types.table, t);
    if (tgt == nullptr) {
        LOGW("target type %s does not exist\n", t);
        return false;
    }
    cls = hashtab_find(db->p_classes.table, c);
    if (cls == nullptr) {
        LOGW("class %s does not exist\n", c);
        return false;
    }
    def = hashtab_find(db->p_types.table, d);
    if (def == nullptr) {
        LOGW("default type %s does not exist\n", d);
        return false;
    }

    avtab_key_t key;
    key.source_type = src->s.value;
    key.target_type = tgt->s.value;
    key.target_class = cls->s.value;
    key.specified = effect;

    avtab_ptr_t node = get_avtab_node(&key, nullptr);
    node->datum.data = def->s.value;

    return true;
}

bool sepol_impl::add_filename_trans(const char *s, const char *t, const char *c, const char *d, const char *o) {
    type_datum_t *src, *tgt, *def;
    class_datum_t *cls;

    src = hashtab_find(db->p_types.table, s);
    if (src == nullptr) {
        LOGW("source type %s does not exist\n", s);
        return false;
    }
    tgt = hashtab_find(db->p_types.table, t);
    if (tgt == nullptr) {
        LOGW("target type %s does not exist\n", t);
        return false;
    }
    cls = hashtab_find(db->p_classes.table, c);
    if (cls == nullptr) {
        LOGW("class %s does not exist\n", c);
        return false;
    }
    def = hashtab_find(db->p_types.table, d);
    if (def == nullptr) {
        LOGW("default type %s does not exist\n", d);
        return false;
    }

    filename_trans_key_t key;
    key.ttype = tgt->s.value;
    key.tclass = cls->s.value;
    key.name = (char *) o;

    filename_trans_datum_t *last = nullptr;
    filename_trans_datum_t *trans = hashtab_find(db->filename_trans, (hashtab_key_t) &key);
    while (trans) {
        if (ebitmap_get_bit(&trans->stypes, src->s.value - 1)) {
            // Duplicate, overwrite existing data and return
            trans->otype = def->s.value;
            return true;
        }
        if (trans->otype == def->s.value)
            break;
        last = trans;
        trans = trans->next;
    }

    if (trans == nullptr) {
        trans = auto_cast(calloc(sizeof(*trans), 1));
        filename_trans_key_t *new_key = auto_cast(malloc(sizeof(*new_key)));
        *new_key = key;
        new_key->name = strdup(key.name);
        trans->next = last;
        trans->otype = def->s.value;
        hashtab_insert(db->filename_trans, (hashtab_key_t) new_key, trans);
    }

    db->filename_trans_count++;
    return ebitmap_set_bit(&trans->stypes, src->s.value - 1, 1) == 0;
}

bool sepol_impl::add_genfscon(const char *fs_name, const char *path, const char *context) {
    // First try to create context
    context_struct_t *ctx;
    if (context_from_string(nullptr, db, &ctx, context, strlen(context))) {
        LOGW("Failed to create context from string [%s]\n", context);
        return false;
    }

    // Allocate genfs context
    ocontext_t *newc = auto_cast(calloc(sizeof(*newc), 1));
    newc->u.name = strdup(path);
    memcpy(&newc->context[0], ctx, sizeof(*ctx));
    free(ctx);

    // Find or allocate genfs
    genfs_t *last_gen = nullptr;
    genfs_t *newfs = nullptr;
    for (genfs_t *node = db->genfs; node; node = node->next) {
        if (strcmp(node->fstype, fs_name) == 0) {
            newfs = node;
            break;
        }
        last_gen = node;
    }
    if (newfs == nullptr) {
        newfs = auto_cast(calloc(sizeof(*newfs), 1));
        newfs->fstype = strdup(fs_name);
        // Insert
        if (last_gen)
            last_gen->next = newfs;
        else
            db->genfs = newfs;
    }

    // Insert or replace genfs context
    ocontext_t *last_ctx = nullptr;
    for (ocontext_t *node = newfs->head; node; node = node->next) {
        if (strcmp(node->u.name, path) == 0) {
            // Unlink
            if (last_ctx)
                last_ctx->next = node->next;
            else
                newfs->head = nullptr;
            // Destroy old node
            free(node->u.name);
            context_destroy(&node->context[0]);
            free(node);
            break;
        }
        last_ctx = node;
    }
    // Insert
    if (last_ctx)
        last_ctx->next = newc;
    else
        newfs->head = newc;

    return true;
}

bool sepol_impl::add_type(const char *type_name, uint32_t flavor) {
    type_datum_t *type = hashtab_find(db->p_types.table, type_name);
    if (type) {
        LOGW("Type %s already exists\n", type_name);
        return true;
    }

    type = auto_cast(malloc(sizeof(type_datum_t)));
    type_datum_init(type);
    type->primary = 1;
    type->flavor = flavor;

    uint32_t value = 0;
    if (symtab_insert(db, SYM_TYPES, strdup(type_name), type, SCOPE_DECL, 1, &value))
        return false;
    type->s.value = value;
    ebitmap_set_bit(&db->global->branch_list->declared.p_types_scope, value - 1, 1);

    auto new_size = sizeof(ebitmap_t) * db->p_types.nprim;
    db->type_attr_map = auto_cast(realloc(db->type_attr_map, new_size));
    db->attr_type_map = auto_cast(realloc(db->attr_type_map, new_size));
    ebitmap_init(&db->type_attr_map[value - 1]);
    ebitmap_init(&db->attr_type_map[value - 1]);
    ebitmap_set_bit(&db->type_attr_map[value - 1], value - 1, 1);

    // Re-index stuffs
    if (policydb_index_decls(nullptr, db) ||
        policydb_index_classes(db) || policydb_index_others(nullptr, db, 0))
        return false;

    // Add the type to all roles
    for (int i = 0; i < db->p_roles.nprim; ++i) {
        // Not sure all those three calls are needed
        ebitmap_set_bit(&db->role_val_to_struct[i]->types.negset, value - 1, 0);
        ebitmap_set_bit(&db->role_val_to_struct[i]->types.types, value - 1, 1);
        type_set_expand(&db->role_val_to_struct[i]->types, &db->role_val_to_struct[i]->cache, db, 0);
    }

    return true;
}

bool sepol_impl::set_type_state(const char *type_name, bool permissive) {
    type_datum_t *type;
    if (type_name == nullptr) {
        hashtab_for_each(db->p_types.table, [&](hashtab_ptr_t node) {
            type = auto_cast(node->datum);
            if (ebitmap_set_bit(&db->permissive_map, type->s.value, permissive))
                LOGW("Could not set bit in permissive map\n");
        });
    } else {
        type = hashtab_find(db->p_types.table, type_name);
        if (type == nullptr) {
            LOGW("type %s does not exist\n", type_name);
            return false;
        }
        if (ebitmap_set_bit(&db->permissive_map, type->s.value, permissive)) {
            LOGW("Could not set bit in permissive map\n");
            return false;
        }
    }
    return true;
}

void sepol_impl::add_typeattribute(type_datum_t *type, type_datum_t *attr) {
    ebitmap_set_bit(&db->type_attr_map[type->s.value - 1], attr->s.value - 1, 1);
    ebitmap_set_bit(&db->attr_type_map[attr->s.value - 1], type->s.value - 1, 1);

    hashtab_for_each(db->p_classes.table, [&](hashtab_ptr_t node){
        auto cls = static_cast<class_datum_t *>(node->datum);
        list_for_each(cls->constraints, [&](constraint_node_t *n) {
            list_for_each(n->expr, [&](constraint_expr_t *e) {
                if (e->expr_type == CEXPR_NAMES &&
                    ebitmap_get_bit(&e->type_names->types, attr->s.value - 1)) {
                    ebitmap_set_bit(&e->names, type->s.value - 1, 1);
                }
            });
        });
    });
}

bool sepol_impl::add_typeattribute(const char *type, const char *attr) {
    type_datum_t *type_d = hashtab_find(db->p_types.table, type);
    if (type_d == nullptr) {
        LOGW("type %s does not exist\n", type);
        return false;
    } else if (type_d->flavor == TYPE_ATTRIB) {
        LOGW("type %s is an attribute\n", attr);
        return false;
    }

    type_datum *attr_d = hashtab_find(db->p_types.table, attr);
    if (attr_d == nullptr) {
        LOGW("attribute %s does not exist\n", type);
        return false;
    } else if (attr_d->flavor != TYPE_ATTRIB) {
        LOGW("type %s is not an attribute \n", attr);
        return false;
    }

    add_typeattribute(type_d, attr_d);
    return true;
}

void sepol_impl::strip_dontaudit() {
    avtab_for_each(&db->te_avtab, [=, this](avtab_ptr_t node) {
        if (node->key.specified == AVTAB_AUDITDENY || node->key.specified == AVTAB_XPERMS_DONTAUDIT)
            avtab_remove_node(&db->te_avtab, node);
    });
}

void sepolicy::print_rules() {
    hashtab_for_each(impl->db->p_types.table, [&](hashtab_ptr_t node) {
        type_datum_t *type = auto_cast(node->datum);
        if (type->flavor == TYPE_ATTRIB) {
            impl->print_type(stdout, type);
        }
    });
    hashtab_for_each(impl->db->p_types.table, [&](hashtab_ptr_t node) {
        type_datum_t *type = auto_cast(node->datum);
        if (type->flavor == TYPE_TYPE) {
            impl->print_type(stdout, type);
        }
    });
    avtab_for_each(&impl->db->te_avtab, [&](avtab_ptr_t node) {
        impl->print_avtab(stdout, node);
    });
    hashtab_for_each(impl->db->filename_trans, [&](hashtab_ptr_t node) {
        impl->print_filename_trans(stdout, node);
    });
    list_for_each(impl->db->genfs, [&](genfs_t *genfs) {
        list_for_each(genfs->head, [&](ocontext *context) {
            char *ctx = nullptr;
            size_t len = 0;
            if (context_to_string(nullptr, impl->db, &context->context[0], &ctx, &len) == 0) {
                fprintf(stdout, "genfscon %s %s %s\n", genfs->fstype, context->u.name, ctx);
                free(ctx);
            }
        });
    });
}

void sepol_impl::print_type(FILE *fp, type_datum_t *type) {
    const char *name = db->p_type_val_to_name[type->s.value - 1];
    if (name == nullptr)
        return;
    if (type->flavor == TYPE_ATTRIB) {
        fprintf(fp, "attribute %s\n", name);
    } else if (type->flavor == TYPE_TYPE) {
        bool first = true;
        ebitmap_t *bitmap = &db->type_attr_map[type->s.value - 1];
        for (uint32_t i = 0; i <= bitmap->highbit; ++i) {
            if (ebitmap_get_bit(bitmap, i)) {
                auto attr_type = db->type_val_to_struct[i];
                if (attr_type->flavor == TYPE_ATTRIB) {
                    if (const char *attr = db->p_type_val_to_name[i]) {
                        if (first) {
                            fprintf(fp, "type %s {", name);
                            first = false;
                        }
                        fprintf(fp, " %s", attr);
                    }
                }
            }
        }
        if (!first) {
            fprintf(fp, " }\n");
        }
    }
    if (ebitmap_get_bit(&db->permissive_map, type->s.value)) {
        fprintf(stdout, "permissive %s\n", name);
    }
}

void sepol_impl::print_avtab(FILE *fp, avtab_ptr_t node) {
    const char *src = db->p_type_val_to_name[node->key.source_type - 1];
    const char *tgt = db->p_type_val_to_name[node->key.target_type - 1];
    const char *cls = db->p_class_val_to_name[node->key.target_class - 1];
    if (src == nullptr || tgt == nullptr || cls == nullptr)
        return;

    if (node->key.specified & AVTAB_AV) {
        uint32_t data = node->datum.data;
        const char *name;
        switch (node->key.specified) {
            case AVTAB_ALLOWED:
                name = "allow";
                break;
            case AVTAB_AUDITALLOW:
                name = "auditallow";
                break;
            case AVTAB_AUDITDENY:
                name = "dontaudit";
                // Invert the rules for dontaudit
                data = ~data;
                break;
            default:
                return;
        }

        class_datum_t *clz = db->class_val_to_struct[node->key.target_class - 1];
        if (clz == nullptr)
            return;

        auto it = class_perm_names.find(cls);
        if (it == class_perm_names.end()) {
            it = class_perm_names.try_emplace(cls).first;
            // Find all permission names and cache the value
            hashtab_for_each(clz->permissions.table, [&](hashtab_ptr_t node) {
                perm_datum_t *perm = auto_cast(node->datum);
                it->second[perm->s.value - 1] = node->key;
            });
            if (clz->comdatum) {
                hashtab_for_each(clz->comdatum->permissions.table, [&](hashtab_ptr_t node) {
                    perm_datum_t *perm = auto_cast(node->datum);
                    it->second[perm->s.value - 1] = node->key;
                });
            }
        }

        bool first = true;
        for (int i = 0; i < 32; ++i) {
            if (data & (1u << i)) {
                if (const char *perm = it->second[i]) {
                    if (first) {
                        fprintf(fp, "%s %s %s %s {", name, src, tgt, cls);
                        first = false;
                    }
                    fprintf(fp, " %s", perm);
                }
            }
        }
        if (!first) {
            fprintf(fp, " }\n");
        }
    } else if (node->key.specified & AVTAB_TYPE) {
        const char *name;
        switch (node->key.specified) {
            case AVTAB_TRANSITION:
                name = "type_transition";
                break;
            case AVTAB_MEMBER:
                name = "type_member";
                break;
            case AVTAB_CHANGE:
                name = "type_change";
                break;
            default:
                return;
        }
        if (const char *def = db->p_type_val_to_name[node->datum.data - 1]) {
            fprintf(fp, "%s %s %s %s %s\n", name, src, tgt, cls, def);
        }
    } else if (node->key.specified & AVTAB_XPERMS) {
        const char *name;
        switch (node->key.specified) {
            case AVTAB_XPERMS_ALLOWED:
                name = "allowxperm";
                break;
            case AVTAB_XPERMS_AUDITALLOW:
                name = "auditallowxperm";
                break;
            case AVTAB_XPERMS_DONTAUDIT:
                name = "dontauditxperm";
                break;
            default:
                return;
        }
        avtab_extended_perms_t *xperms = node->datum.xperms;
        if (xperms == nullptr)
            return;

        vector<pair<uint8_t, uint8_t>> ranges;
        {
            int low = -1;
            for (int i = 0; i < 256; ++i) {
                if (xperm_test(i, xperms->perms)) {
                    if (low < 0) {
                        low = i;
                    }
                    if (i == 255) {
                        ranges.emplace_back(low, 255);
                    }
                } else if (low >= 0) {
                    ranges.emplace_back(low, i - 1);
                    low = -1;
                }
            }
        }

        auto to_value = [&](uint8_t val) -> uint16_t {
            if (xperms->specified == AVTAB_XPERMS_IOCTLFUNCTION) {
                return (((uint16_t) xperms->driver) << 8) | val;
            } else {
                return ((uint16_t) val) << 8;
            }
        };

        if (!ranges.empty()) {
            fprintf(fp, "%s %s %s %s ioctl {", name, src, tgt, cls);
            for (auto [l, h] : ranges) {
                uint16_t low = to_value(l);
                uint16_t high = to_value(h);
                if (low == high) {
                    fprintf(fp, " 0x%04X", low);
                } else {
                    fprintf(fp, " 0x%04X-0x%04X", low, high);
                }
            }
            fprintf(fp, " }\n");
        }
    }
}

void sepol_impl::print_filename_trans(FILE *fp, hashtab_ptr_t node) {
    auto key = reinterpret_cast<filename_trans_key_t *>(node->key);
    filename_trans_datum_t *trans = auto_cast(node->datum);

    const char *tgt = db->p_type_val_to_name[key->ttype - 1];
    const char *cls = db->p_class_val_to_name[key->tclass - 1];
    const char *def = db->p_type_val_to_name[trans->otype - 1];
    if (tgt == nullptr || cls == nullptr || def == nullptr || key->name == nullptr)
        return;

    for (uint32_t i = 0; i <= trans->stypes.highbit; ++i) {
        if (ebitmap_get_bit(&trans->stypes, i)) {
            if (const char *src = db->p_type_val_to_name[i]) {
                fprintf(fp, "type_transition %s %s %s %s %s\n", src, tgt, cls, def, key->name);
            }
        }
    }
}
