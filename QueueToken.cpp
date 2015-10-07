// QueueToken.cpp

#include "pch.h"


AccessQueue::QueueToken::QueueToken(const std::pair<unsigned int, bool> * Identifier, AccessQueue *ParentQueue)
{
  this->Identifier = Identifier;
  this->ParentQueue = ParentQueue;
  this->Prio = Identifier->first;
  this->WriteAccess = Identifier->second;
}

AccessQueue::QueueToken::~QueueToken()
{
  if(Identifier)
    ParentQueue->Unlock(Identifier);
}

int AccessQueue::QueueToken::GetQueueLength()
{
  return ParentQueue->GetQueueLength();
}

int AccessQueue::QueueToken::GetPrioQueueLength()
{
  return ParentQueue->GetQueueLength(*this);
}

const std::pair<unsigned int, bool> * AccessQueue::QueueToken::GetIdentifier() const
{
  return Identifier;
}

void AccessQueue::QueueToken::Unlock()
{
  if(Identifier)
  {
    ParentQueue->Unlock(Identifier);
    Identifier = NULL;
  }
}

void AccessQueue::QueueToken::Relock()
{
  if(!Identifier)
  {
    *this = ParentQueue->Lock(Prio, WriteAccess);
  }
}

void AccessQueue::QueueToken::Yield()
{
  if(ParentQueue->GetQueueLength(*this))
  {
    ParentQueue->Yield(*this);
  }
}
