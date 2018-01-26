/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <iostream>
#include "wrench/util/MessageManager.h"


namespace wrench{

    // TODO: At some point, we may want to make this with only unique pointers...

    std::map<std::string,std::vector<SimulationMessage*>> MessageManager::mailbox_messages = {};

    void MessageManager::manageMessage(std::string mailbox, SimulationMessage *msg) {
        if (mailbox_messages.find(mailbox) == mailbox_messages.end() ) {
            mailbox_messages.insert({mailbox, {}});
        }
        mailbox_messages[mailbox].push_back(msg);
    }

    void MessageManager::cleanUpMessages(std::string mailbox) {
        std::map<std::string,std::vector<SimulationMessage*>>::iterator msg_itr;
        for(msg_itr = mailbox_messages.begin();msg_itr!=mailbox_messages.end();msg_itr++){
            if((*msg_itr).first==mailbox){
                for(int i=0;i<(*msg_itr).second.size();i++){
                    delete (*msg_itr).second[i];
                }
                (*msg_itr).second.clear();
            }
        }
    }

    void MessageManager::removeReceivedMessages(std::string mailbox, SimulationMessage *msg) {
        std::map<std::string,std::vector<SimulationMessage*>>::iterator msg_itr;
        for(msg_itr = mailbox_messages.begin();msg_itr!=mailbox_messages.end();msg_itr++){
            if((*msg_itr).first==mailbox){
                std::vector<SimulationMessage*>::iterator it;
                for(it=(*msg_itr).second.begin();it!=(*msg_itr).second.end();it++){
                    if((*it)==msg){
                        it = (*msg_itr).second.erase(it);
                        break;
                    }
                }
            }
        }
    }


}
