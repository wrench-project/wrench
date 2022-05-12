/**
* Copyright (c) 2017. The WRENCH Team.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*/

#include "wrench/services/storage/xrootd/XRootDMessage.h"
//
// Created by jamcdonald on 3/28/22.
//

/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */



namespace wrench {
    namespace XRootD{
            Message::Message(double payload):ServiceMessage(payload){}
            FileSearchRequestMessage::FileSearchRequestMessage(simgrid::s4u::Mailbox *answer_mailbox,
                                     std::shared_ptr<DataFile> file,
                                     double payload):Message(payload),answer_mailbox(answer_mailbox),file(file){}


            FileSearchAnswerMessage::FileSearchAnswerMessage(std::shared_ptr<DataFile> file,
                                    std::shared_ptr<FileLocation> location,
                                    bool success,
                                    std::shared_ptr<FailureCause> failure_cause,
                                    double payload):Message(payload),file(file),location(location),success(success),failure_cause(failure_cause){}

            ContinueSearchMessage::ContinueSearchMessage(simgrid::s4u::Mailbox *answer_mailbox,
                                  std::shared_ptr<DataFile> file,
                                  Node* node,
                                  double payload,
                                  std::shared_ptr<bool> answered,
                                  int timeToLive):Message(payload),file(file),node(node),answered(answered),timeToLive(timeToLive){}
            ContinueSearchMessage::ContinueSearchMessage(ContinueSearchMessage* other):Message(payload),file(other->file),node(other->node),answered(other->answered),timeToLive(other->timeToLive-1){}


            UpdateCacheMessage::UpdateCacheMessage(simgrid::s4u::Mailbox *answer_mailbox,Node* node,std::shared_ptr<DataFile> file,  std::set<std::shared_ptr<FileLocation>> locations,
                               double payload, std::shared_ptr<bool> answered):Message(payload),answer_mailbox(answer_mailbox),node(node),file(file),locations(locations),answered(answered){}

            FileDeleteRequestMessage::FileDeleteRequestMessage(              std::shared_ptr<DataFile> file,
                                     double payload,int timeToLive):Message(payload),file(file),timeToLive(timeToLive){}
            FileDeleteRequestMessage::FileDeleteRequestMessage(FileDeleteRequestMessage* other):Message(payload),file(other->file),timeToLive(other->timeToLive-1){}

/
    }
};// namespace wrench

