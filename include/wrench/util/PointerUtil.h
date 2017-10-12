/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#ifndef WRENCH_POINTERUTIL_H
#define WRENCH_POINTERUTIL_H


#include <memory>
#include <set>

namespace wrench {

    class PointerUtil {

    public:

        template<class T>
        static void moveUniquePtrFromSetToSet(
                typename std::set<std::unique_ptr<T>>::iterator it,
                                                 std::set<std::unique_ptr<T>> *from,
                                                 std::set<std::unique_ptr<T>> *to)
        {

          auto tmp = const_cast<std::unique_ptr<T>&&>(*it);
          (*from).erase(it);
          (*to).insert(std::move(tmp));

        };
    };

};


#endif //WRENCH_POINTERUTIL_H
