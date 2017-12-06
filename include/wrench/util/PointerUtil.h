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
#include <deque>

namespace wrench {

    /***********************/
    /** \cond INTERNAL     */
    /***********************/

    /**
     * @brief A helper class that implements useful smart pointer operations
     */
    class PointerUtil {

    public:

        /**
         * @brief A helper method to move a unique_ptr from a set to another
         * @tparam T: template class
         * @param ptr: unique pointer to an object of class T that's in a set
         * @param from: pointer to the set in which the object is
         * @param to: pointer to the set to which the object should be moved
         */
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

        template<class T>
        static void moveUniquePtrFromDequeToSet(
                typename std::deque<std::unique_ptr<T>>::iterator it,
                std::deque<std::unique_ptr<T>> *from,
                std::set<std::unique_ptr<T>> *to)
        {

            auto tmp = const_cast<std::unique_ptr<T>&&>(*it);
            (*from).erase(it);
            (*to).insert(std::move(tmp));

        };

        template <class T1>
        static void moveSingleSeparateUniquePtrFromSetToSet(std::unique_ptr<T1>* ptr,
                                                           std::set<std::unique_ptr<T1>> *from,
                                                           std::set<std::unique_ptr<T1>> *to);
    };

    /***********************/
    /** \endcond           */
    /***********************/


};


#endif //WRENCH_POINTERUTIL_H
