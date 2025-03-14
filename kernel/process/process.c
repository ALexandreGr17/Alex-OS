#include "process.h"

struct process_context ctx;

void save_ctx() {
    i686_save_context(&ctx);
}

void load_ctx() {
    i686_load_context(&ctx);
}
