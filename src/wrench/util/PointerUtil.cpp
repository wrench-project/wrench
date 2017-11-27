/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */


#include <wrench/services/compute/standard_job_executor/StandardJobExecutor.h>
#include "wrench/util/PointerUtil.h"

namespace wrench {

    /***********************/
    /** \cond INTERNAL    */
    /***********************/


    /**
     * @brief An internal helper method to move a unique_ptr from a set to another
     * @tparam T: template class
     * @param ptr: unique pointer to an object of class T that's in a set
     * @param from: pointer to the set in which the object is
     * @param to: pointer to the set to which the object should be moved
     */
    template <class T>
    void PointerUtil::moveSingleSeparateUniquePtrFromSetToSet(std::unique_ptr<T>* ptr,
                                                       std::set<std::unique_ptr<T>> *from,
                                                       std::set<std::unique_ptr<T>> *to){
      auto tmp = const_cast<std::unique_ptr<T>&&>(*ptr);
      (*from).erase(*ptr);
      (*to).insert(std::move(tmp));
    };


    /***********************/
    /** \endcond           */
    /***********************/

};