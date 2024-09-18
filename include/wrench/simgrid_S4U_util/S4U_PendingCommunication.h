/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef WRENCH_S4U_PENDINGCOMMUNICATION_H
#define WRENCH_S4U_PENDINGCOMMUNICATION_H

#include "wrench/simulation/SimulationMessage.h"
#include "wrench/util/MessageManager.h"
#include <simgrid/s4u/Comm.hpp>
#include <vector>

namespace wrench {

/*******************/
/** \cond INTERNAL */
/*******************/

/** @brief This is a simple wrapper class around S4U asynchronous communication
 * checking methods */
class S4U_PendingCommunication {
public:
  /**
   * @brief The communication operation's type
   */
  enum OperationType { SENDING, RECEIVING };

  /**
   * @brief Constructor
   *
   * @param commport: the CommPort
   * @param operation_type: the operation type
   */
  S4U_PendingCommunication(S4U_CommPort *commport, OperationType operation_type)
      : commport(commport), operation_type(operation_type) {}

  std::unique_ptr<SimulationMessage> wait();
  std::unique_ptr<SimulationMessage> wait(double timeout);

  static unsigned long waitForSomethingToHappen(
      const std::vector<std::shared_ptr<S4U_PendingCommunication>>
          &pending_comms,
      double timeout);

  static unsigned long waitForSomethingToHappen(
      std::vector<S4U_PendingCommunication *> pending_comms, double timeout);

  //        ~S4U_PendingCommunication() default;

  /** @brief The message */
  std::unique_ptr<SimulationMessage> simulation_message;
  /** @brief The CommPort */
  S4U_CommPort *commport;
  /** @brief The operation type */
  OperationType operation_type;

  /** @brief The SimGrid Mailbox communication handle */
  simgrid::s4u::CommPtr comm_ptr;
  /** @brief The SimGrid MessageQueue communication handle */
  simgrid::s4u::MessPtr mess_ptr;
};

/*******************/
/** \endcond */
/*******************/

} // namespace wrench

#endif // WRENCH_S4U_PENDINGCOMMUNICATION_H
