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
         * @tparam template class
         * @param it: iterator
         * @param from: pointer to the set in which the object is
         * @param to: pointer to the set to which the object should be moved
         */
        template<class T>
        static void moveUniquePtrFromSetToSet(
                typename std::set<std::unique_ptr<T>>::iterator it,
                std::set<std::unique_ptr<T>> *from,
                std::set<std::unique_ptr<T>> *to) {

            auto tmp = const_cast<std::unique_ptr<T> &&>(*it);
            (*from).erase(it);
            (*to).insert(std::move(tmp));
        };

        /**
         * @brief A helper method to move a shared_ptr from a set to another
         * @tparam template class
         * @param it: iterator
         * @param from: pointer to the set in which the object is
         * @param to: pointer to the set to which the object should be moved
         */
        template<class T>
        static void moveSharedPtrFromSetToSet(
                typename std::set<std::shared_ptr<T>>::iterator it,
                std::set<std::shared_ptr<T>> *from,
                std::set<std::shared_ptr<T>> *to) {

            auto tmp = const_cast<std::shared_ptr<T> &&>(*it);
            (*from).erase(it);
            (*to).insert(tmp);
        };

#if 0
        /**
         * @brief A helper method to move a unique_ptr from a dequeue to a set
         * @tparam template class
         * @param it: iterator
         * @param from: pointer to the dequeue in which the object is
         * @param to: pointer to the set to which the object should be moved
         */
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


        /**
         * @brief A helper method to move a unique_ptr from a set to another
         * @tparam template class
         * @param ptr: the object to move
         * @param from: pointer to the set in which the object is
         * @param to: pointer to the set to which the object should be moved
         */
        template <class T1>
        static void moveSingleSeparateUniquePtrFromSetToSet(std::unique_ptr<T1>* ptr,
                                                           std::set<std::unique_ptr<T1>> *from,
                                                           std::set<std::unique_ptr<T1>> *to);

#endif
    };

    /***********************/
    /** \endcond           */
    /***********************/


};// namespace wrench


#endif//WRENCH_POINTERUTIL_H
