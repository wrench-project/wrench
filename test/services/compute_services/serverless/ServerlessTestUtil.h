
#ifndef SERVERLESSTESTUTIL_H
#define SERVERLESSTESTUTIL_H

#include <memory>

#include "wrench-dev.h"

static std::function<void(const std::shared_ptr<wrench::StorageService>&)> noop_init =
        [](const std::shared_ptr<wrench::StorageService>& storage_service) -> void {};

#endif //SERVERLESSTESTUTIL_H
