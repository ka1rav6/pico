pub const Event = enum {
    file_hash_generated,
    file_recompiled,
    file_dependency_checked,
    file_new_compiled,
    graph_new_nodes_created,
    graph_structure_modified,
    directory_scanned,
};
