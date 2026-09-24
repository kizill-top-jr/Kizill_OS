include/kernel/

    types.h — u8..u64, size_t, NULL.

    limine.h — Limine boot protocol structs + request IDs. All in one
    place so main.c and pmm.c can share.

    Rest is 1:1 with kernel/*.c.
    EOF

cat > docs/README.md << 'EOF'
docs/

Random notes, design decisions, TODOs. Nothing here yet — will fill
as project grows.
