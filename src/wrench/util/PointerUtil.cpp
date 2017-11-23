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

//    template <class T>
//    void PointerUtil::moveUniquePtrFromSetToSet(
//                          typename std::set<std::unique_ptr<T>>::iterator it,
//                          std::set<std::unique_ptr<T>> *from,
//                          std::set<std::unique_ptr<T>> *to) {
//
//      auto tmp = const_cast<std::unique_ptr<T>&&>(*it);
//      (*from).erase(it);
//      (*to).insert(std::move(tmp));
//
//    }

    template <class T>
    void PointerUtil::moveSingleSeparateUniquePtrFromSetToSet(std::unique_ptr<T>* ptr,
                                                       std::set<std::unique_ptr<T>> *from,
                                                       std::set<std::unique_ptr<T>> *to){
      auto tmp = const_cast<std::unique_ptr<T>&&>(*ptr);
      (*from).erase(*ptr);
      (*to).insert(std::move(tmp));
    };

    template void PointerUtil::moveSingleSeparateUniquePtrFromSetToSet<StandardJobExecutor>(std::unique_ptr<StandardJobExecutor> *ptr,
                                                                      std::set<std::unique_ptr<StandardJobExecutor>> *from,
                                                                      std::set<std::unique_ptr<StandardJobExecutor>> *to);

    /***********************/
    /** \endcond           */
    /***********************/

};