//  Copyright 2020 Fraunhofer-Gesellschaft zur Förderung der angewandten Forschung e.V.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//  https:www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//


#include "mars_common/behavior/BlackBoardAccessControl.h"

#include <map>

static std::map<std::string, BT::BlackboardAccessElementManager*> gBlackboardAccessElementManager;

static const mars::common::Id INVALID_ID = mars::common::Id::createInvalidId();

template <typename T>
bool BT::BlackBoardAccessControl::readFromBlackboard(T& pBlackboardValue,
                                                     const std::string& pBlackboardKey,
                                                     const BT::TreeNode& pTreeNode,
                                                     const mars::common::Id& pClassId,
                                                     BT::ValueAccessMode pValueAccessMode)
{
  bool lErrorOccurred = false;
  std::mutex mutex;
  BT::BlackboardAccessElementManager* lBlackboardAccessElementManager;

  lBlackboardAccessElementManager =
      BT::BlackBoardAccessControl::getBlackboardAccessElementManager(pBlackboardKey);

  if ((pValueAccessMode == BT::ValueAccessMode::READ_AND_LOCK_NOT_BLOCKING) ||
      (pValueAccessMode == BT::ValueAccessMode::READ_ONLY))
  {
    if (pValueAccessMode == BT::ValueAccessMode::READ_AND_LOCK_NOT_BLOCKING)
    {
      if (lBlackboardAccessElementManager->tryLock())
      {
        lBlackboardAccessElementManager->lock(pClassId);
      }
      else
      {
        lErrorOccurred = true;
      }
    }
  }
  else
  {
    lBlackboardAccessElementManager->lock(pClassId);
  }

  if (!lErrorOccurred)
  {
    // read from blackboard
    pBlackboardValue = pTreeNode.getInput<T>(pBlackboardKey);
  }

  return !lErrorOccurred;
}

template <typename T>
bool BT::BlackBoardAccessControl::writeToBlackboard(const T& pBlackboardValue,
                                                    const std::string& pBlackboardKey,
                                                    const BT::TreeNode& pTreeNode,
                                                    const mars::common::Id& pClassId)
{
  BT::BlackboardAccessElementManager* lBlackboardAccessElementManager;
  bool lErrorOccurred = false;

  lBlackboardAccessElementManager =
      BT::BlackBoardAccessControl::getBlackboardAccessElementManager(pBlackboardKey);

  if (lBlackboardAccessElementManager->getCurrentOwnerId() != INVALID_ID)
  {
    lErrorOccurred = lBlackboardAccessElementManager->getCurrentOwnerId() != pClassId;
  }

  if (!lErrorOccurred)
  {
    lErrorOccurred = !pTreeNode.setOutput<T>(pBlackboardKey, pBlackboardValue);
  }
  

  return !lErrorOccurred;
}

bool BT::BlackBoardAccessControl::unlockBlackboardParam(const std::string& pBlackboardKey,
                                                        const mars::common::Id& pClassId)
{
  BT::BlackboardAccessElementManager* lBlackboardAccessElementManager;

  lBlackboardAccessElementManager =
      BT::BlackBoardAccessControl::getBlackboardAccessElementManager(pBlackboardKey);
  
  return lBlackboardAccessElementManager->unlock(pClassId);
}

BT::BlackboardAccessElementManager*
BT::BlackBoardAccessControl::getBlackboardAccessElementManager(const std::string& pBlackboardKey)
{
  std::map<std::string, BT::BlackboardAccessElementManager*>::iterator it;
  BT::BlackboardAccessElementManager* lBlackboardAccessElementManager;

  it = gBlackboardAccessElementManager.find(pBlackboardKey);

  if (it != gBlackboardAccessElementManager.end())
  {
    lBlackboardAccessElementManager = it->second;
  }
  else
  {
    lBlackboardAccessElementManager = new BT::BlackboardAccessElementManager();

    gBlackboardAccessElementManager.insert(
        std::pair<std::string, BT::BlackboardAccessElementManager*>(
            pBlackboardKey, lBlackboardAccessElementManager));
  }

  return lBlackboardAccessElementManager;
}